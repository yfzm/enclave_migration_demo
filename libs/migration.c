#include "stdio.h"
#include "string.h"
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
extern int migration_flag;

unsigned long mcode_pages = 0;
unsigned long mdata_pages = 0;
unsigned long mheap_pages = 0;
unsigned long mstack_pages = 0;
unsigned long mthread_pages = 0;

#define METADATA_ADDR 0x600000000000
#define METADATA_DATA 0
#define METADATA_MALLOC 1
#define METADATA_MMAP 2
#define METADATA_STACK 3
#define METADATA_THREAD 4

#undef YFZM_DEBUG
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
	int base = 8 + type * 16;
	*(unsigned long *)(METADATA_ADDR + base) = offset;
	*(unsigned long *)(METADATA_ADDR + base + 8) = size;
}

void dump_out(char *out)
{
	char *addr;
	char *target;
	unsigned long i;
	int j;
	unsigned long offset;
	unsigned long enclave_start_addr;

	unsigned long data_size;
	unsigned long malloc_size;
	unsigned long mmap_size; 
	unsigned long stack_size;

	enclave_start_addr = (unsigned long)&enclave_start;

    data_size = 0x800000;  // 8MB
	malloc_size = __brk - __init_brk;
	mmap_size = __cur_mmap - __init_brk - MALLOC_AREA_SIZE;
	stack_size = 0x800000 * N_THREADS;
	
	//store metadata (used in sdk/client.c)
	*(unsigned long *)(METADATA_ADDR) = N_THREADS;

	printf("total dumpped size: %lx bytes\n", data_size + malloc_size + mmap_size + stack_size + 3 * N_THREADS * PS);
	
	//dump data section
	offset = PS * mcode_pages;
	addr = (char*)(enclave_start_addr + offset);
	target = out + offset;
    yfzm_printf("[data] target: %p, addr: %p, size: %lx\n", target, addr, data_size);
	set_metadata(METADATA_DATA, offset, data_size);
	memcpy(target, addr, data_size);

	//dump malloc area
	offset = PS * (mcode_pages + mdata_pages);
	addr = (char*)(enclave_start_addr + offset);
	target = out + offset;
    yfzm_printf("[allc] target: %p, addr: %p, size: %lx\n", target, addr, malloc_size);
	set_metadata(METADATA_MALLOC, offset, malloc_size);
	memcpy(target, addr, malloc_size);
	
	//dump mmap area
	offset = PS * (mcode_pages + mdata_pages) + MALLOC_AREA_SIZE;
	addr = (char*)(enclave_start_addr + offset);
	target = out + offset;
    yfzm_printf("[mmap] target: %p, addr: %p, size: %lx\n", target, addr, mmap_size);
	set_metadata(METADATA_MMAP, offset, mmap_size);
	memcpy(target, addr, mmap_size);

	//dump stack section
	offset = PS * (mcode_pages + mdata_pages + mheap_pages + mstack_pages) - stack_size;
	addr = (char*)(enclave_start_addr + offset);
	target = out + offset;
    yfzm_printf("[stck] target: %p, addr: %p, size: %lx\n", target, addr, stack_size);
	set_metadata(METADATA_STACK, offset, stack_size);
	memcpy(target, addr, stack_size);
	check_mem_at(0x7f7d0000);
	check_mem_at(0x60003f7d0000);

	//dump thread section
	offset = PS * (mcode_pages + mdata_pages + mheap_pages + mstack_pages);
	addr = (char*)(enclave_start_addr + offset);
	target = out + offset;
	set_metadata(METADATA_THREAD, offset, 3 * N_THREADS * PS);
	for(i = 0; i < 3 * N_THREADS; ++i, addr += PS, target += PS) {
		if(i % 3 != 0)
			memcpy(target, addr, PS);
		else
			memset(target, 0, PS);
	}
}
