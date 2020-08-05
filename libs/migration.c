#include "stdio.h"
#include "string.h"
#include "assert.h"
#include "vars.h"
//dump each section
//code
//data
//heap (ignore)
//stack
//TCS (ignore) & SSA
//TLS

#define PS 0x1000

extern unsigned long enclave_start;
extern unsigned long __init_brk, __brk;
extern unsigned long __cur_mmap;
extern unsigned long init_stack_1;
int __attribute__((weak)) migration_flag;
extern int running_thread_num;  // defined in trampo.c

unsigned long mcode_pages = 0;
unsigned long mdata_pages = 0;
unsigned long mheap_pages = 0;
unsigned long mstack_pages = 0;
unsigned long mthread_pages = 0;

unsigned long data_size;
unsigned long malloc_size;
unsigned long mmap_size; 
unsigned long stack_size;
unsigned long thread_size;

static unsigned long total_size = 0;
static unsigned long sent_size = 0;

unsigned long sizes[5] = {0};
unsigned long offsets[5] = {0};
unsigned long sents[5] = {0};

//unsigned long data_sent = 0;
//unsigned long malloc_sent = 0;
//unsigned long mmap_sent = 0; 
//unsigned long stack_sent = 0;
//unsigned long thread_sent = 0;

#define METADATA_ADDR 0x600000000000
#define METADATA_DATA 0
#define METADATA_MALLOC 1
#define METADATA_MMAP 2
#define METADATA_STACK 3
#define METADATA_THREAD 4

//#define YFZM_DEBUG
#ifdef YFZM_DEBUG
# define yfzm_printf(...) printf(__VA_ARGS__)
# define check_mem_at(addr) printf("[check] %p: %p\n", (addr), *(unsigned long *)(addr));
#else
# define yfzm_printf(...)
# define check_mem_at(addr)
#endif

void set_migration_flag()
{
    migration_flag = 1;
}

void set_metadata(int type, unsigned long offset, unsigned long size) {
	unsigned long base = METADATA_ADDR + 8;
	offsets[type] = offset;
	sizes[type] = size;
	*(unsigned long *)(base + 8 * type) = offset;
	*(unsigned long *)(base + 8 * type + 40) = size;
}

void dump_out_init() {
	yfzm_printf("[dump_out_init]\n");
	int n_threads = running_thread_num - 1;  // exclude helper thread
	char *addr;
	char *target;
	unsigned long i;
	int j;
	unsigned long offset;
	unsigned long enclave_start_addr;

	enclave_start_addr = (unsigned long)&enclave_start;

    data_size = 0x800000;  // 8MB
	malloc_size = __brk - __init_brk;
	mmap_size = __cur_mmap - __init_brk - MALLOC_AREA_SIZE;
	stack_size = 0x800000 * n_threads;
	thread_size = 3 * n_threads * PS;
	
	//store metadata (used in sdk/client.c)
	offset = 0;
	*(unsigned long *)(METADATA_ADDR) = n_threads;
	offset += PS * mcode_pages;
	set_metadata(METADATA_DATA, offset, data_size);
	offset += PS * mdata_pages;
	set_metadata(METADATA_MALLOC, offset, malloc_size);
	set_metadata(METADATA_MMAP, offset + MALLOC_AREA_SIZE, mmap_size);
	offset += PS * (mheap_pages + mstack_pages);
	set_metadata(METADATA_STACK, offset - stack_size, stack_size);
	set_metadata(METADATA_THREAD, offset, thread_size);

	total_size = data_size + malloc_size + mmap_size + stack_size + thread_size;
	printf("total dumpped size: 0x%lx bytes (%f MB)\n", total_size, total_size / 1024.0 / 1024.0);
}

static void yfzm_memcpy(int type, void *dst, void *src, unsigned long size) {
	if (type != METADATA_THREAD) {
		memcpy(dst, src, size);
		return;
	}
	for (unsigned long offset = 0; offset < size; offset += PS) {
		yfzm_printf("  addr %p, mod %d\n", (unsigned long)(src + offset), (unsigned long)(src + offset) % 0x10000 % 0x3000);
		if ((unsigned long)(src + offset) % 0x10000 % 0x3000 != 0)
			memcpy(dst + offset, src + offset, PS);
	}
}

int dump_out(char *out, unsigned long size)
{
	yfzm_printf("[dump_out] size %lx, total %lx, sent %lx, remaining %lx\n", size, total_size, sent_size, total_size - sent_size);
	int finished = 0;
	unsigned long offset = sent_size;
	if (size > total_size - sent_size) {
		size = total_size - sent_size;
		finished = 1;
	}
	sent_size += size;

	for (int type = 0; type < 5; type++) {
		unsigned long remaining_size = sizes[type] - sents[type];
		yfzm_printf("  type %d, remaining_size %lx\n", type, remaining_size);
		if (remaining_size > 0) {
			unsigned long offset = offsets[type] + sents[type];
			yfzm_printf("  size %lx, offset %lx\n", size, offset);
			if (size <= remaining_size) {
				yfzm_printf("    %p <- %p, %lx\n", out + offset, (char *)(&enclave_start) + offset, size);
				yfzm_memcpy(type, out + offset, (char *)(&enclave_start) + offset, size);
				sents[type] += size;
				size = 0;
				break;
			} else {
				yfzm_memcpy(type, out + offset, (char *)(&enclave_start) + offset, remaining_size);
				sents[type] += remaining_size;
				size -= remaining_size;
			}
		}
	}

	yfzm_printf("  return %d\n", finished);
	return finished;

	////dump data section
	//offset = PS * mcode_pages;
	//addr = (char*)(enclave_start_addr + offset);
	//target = out + offset;
    //yfzm_printf("[data] target: %p, addr: %p, size: %lx\n", target, addr, data_size);
	//set_metadata(METADATA_DATA, offset, data_size);
	//memcpy(target, addr, data_size);

	////dump malloc area
	//offset = PS * (mcode_pages + mdata_pages);
	//addr = (char*)(enclave_start_addr + offset);
	//target = out + offset;
    //yfzm_printf("[allc] target: %p, addr: %p, size: %lx\n", target, addr, malloc_size);
	//set_metadata(METADATA_MALLOC, offset, malloc_size);
	//memcpy(target, addr, malloc_size);
	//
	////dump mmap area
	//offset = PS * (mcode_pages + mdata_pages) + MALLOC_AREA_SIZE;
	//addr = (char*)(enclave_start_addr + offset);
	//target = out + offset;
    //yfzm_printf("[mmap] target: %p, addr: %p, size: %lx\n", target, addr, mmap_size);
	//set_metadata(METADATA_MMAP, offset, mmap_size);
	//memcpy(target, addr, mmap_size);

	////dump stack section
	//offset = PS * (mcode_pages + mdata_pages + mheap_pages + mstack_pages) - stack_size;
	//addr = (char*)(enclave_start_addr + offset);
	//target = out + offset;
    //yfzm_printf("[stck] target: %p, addr: %p, size: %lx\n", target, addr, stack_size);
	//set_metadata(METADATA_STACK, offset, stack_size);
	//memcpy(target, addr, stack_size);
	//check_mem_at(0x7f7d0000);
	//check_mem_at(0x60003f7d0000);

	////dump thread section
	//offset = PS * (mcode_pages + mdata_pages + mheap_pages + mstack_pages);
	//addr = (char*)(enclave_start_addr + offset);
	//target = out + offset;
	//set_metadata(METADATA_THREAD, offset, 3 * n_threads * PS);
	//for(i = 0; i < 3 * n_threads; ++i, addr += PS, target += PS) {
	//	if(i % 3 != 0)
	//		memcpy(target, addr, PS);
	//	else
	//		memset(target, 0, PS);
	//}
}
