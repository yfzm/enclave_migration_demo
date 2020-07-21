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

#define ENABLE_COPY_NECESSARY 1
#if ENABLE_COPY_NECESSARY

#define METADATA_ADDR 0x600000000000
#define METADATA_DATA 0
#define METADATA_MALLOC 1
#define METADATA_MMAP 2
#define METADATA_STACK 3
#define METADATA_THREAD 4

#define check_mem_at(addr) printf("[check] %p: %p\n", (addr), *(unsigned long *)(addr));

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
	//int skip_tcs;
	unsigned long enclave_start_addr;

	unsigned long data_size;
	unsigned long malloc_size;
	unsigned long mmap_size; 
	unsigned long stack_size;

	enclave_start_addr = (unsigned long)&enclave_start;

	//code_size = *(unsigned long*)out;
	//data_size = *(((unsigned long*)out)+1);
    data_size = 0x800000;  // 8MB
	malloc_size = __brk - __init_brk;
	mmap_size = __cur_mmap - __init_brk - MALLOC_AREA_SIZE;
	//heap_size = *(((unsigned long*)out)+2) + __brk - __init_brk;
	stack_size = 0x800000 * N_THREADS;
	//stack_size = 0x4000;
	
	//store metadata (used in sdk/client.c)
	*(unsigned long *)(METADATA_ADDR) = N_THREADS;

//	printf("%p: %lx\n", METADATA_ADDR, *(unsigned long *)(METADATA_ADDR));

	//printf("[copy] code 0x%lx, data 0x%lx, heap 0x%lx, stack 0x%lx\n",
	//		code_size, data_size, heap_size, stack_size);

	//dump code section
	//addr = (char*)enclave_start_addr;
	//target = out;
	//memcpy(target, addr, code_size);
	
	//dump data section
	offset = PS * mcode_pages;
	addr = (char*)(enclave_start_addr + offset);
	target = out + offset;
    printf("[data] target: %p, addr: %p, size: %lx\n", target, addr, data_size);
	set_metadata(METADATA_DATA, offset, data_size);
	memcpy(target, addr, data_size);

	//dump malloc area
	offset = PS * (mcode_pages + mdata_pages);
	addr = (char*)(enclave_start_addr + offset);
	target = out + offset;
    printf("[allc] target: %p, addr: %p, size: %lx\n", target, addr, malloc_size);
	set_metadata(METADATA_MALLOC, offset, malloc_size);
	memcpy(target, addr, malloc_size);
	
	//dump mmap area
	offset = PS * (mcode_pages + mdata_pages) + MALLOC_AREA_SIZE;
	addr = (char*)(enclave_start_addr + offset);
	target = out + offset;
    printf("[mmap] target: %p, addr: %p, size: %lx\n", target, addr, mmap_size);
	set_metadata(METADATA_MMAP, offset, mmap_size);
	memcpy(target, addr, mmap_size);

#if 0
	//TODO: multiple threads
	//dump stack section
	addr = ((char*)&init_stack_1) - stack_size;
	offset = (unsigned long)addr - enclave_start_addr;
	target = out + offset;
	memcpy(target, addr, stack_size);
#endif

	//dump stack section
	offset = PS * (mcode_pages + mdata_pages + mheap_pages + mstack_pages) - stack_size;
	addr = (char*)(enclave_start_addr + offset);
	target = out + offset;
    printf("[stck] target: %p, addr: %p, size: %lx\n", target, addr, stack_size);
	set_metadata(METADATA_STACK, offset, stack_size);
	memcpy(target, addr, stack_size);
	check_mem_at(0x7f7d0000);
	check_mem_at(0x60003f7d0000);

	//skip_tcs = 0;
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
#else
void dump_out(char *out)
{
	char *addr;
	char *target;
	unsigned long i;
	int j;
	unsigned long offset;
	int skip_tcs;
	unsigned long enclave_start_addr;

	enclave_start_addr = (unsigned long)&enclave_start;
	//printf("[inside] out: addr 0x%lx\n", (unsigned long)out);

	//dump code section
	addr = (char*)enclave_start_addr;
	target = out;

	//printf("[inside] src: addr 0x%lx, dst: addr 0x%lx\n", (unsigned long)addr, 
//(unsigned long)target);
	//printf("[copy] code 0x%lx, data 0x%lx, heap 0x%lx, stack 0x%lx, thread 0x%lx\n",
	//		mcode_pages, mdata_pages, mheap_pages, mstack_pages, mthread_pages);

	
	memcpy(target, addr, mcode_pages*PS);

#if 0
	for(i = 0; i < mcode_pages; ++i)
	{
		for(j = 0; j < PS; ++j)
			target[j] = addr[j];
		addr += PS;
		target += PS;
	}
#endif

	
	//dump data section
	offset = PS * mcode_pages;
	addr = (char*)(enclave_start_addr + offset);
	target = out + offset;

	memcpy(target, addr, mdata_pages*PS);

#if 0
	for(i = 0; i < mdata_pages; ++i)
	{
		for(j = 0; j < PS; ++j)
			target[j] = addr[j];
		addr += PS;
		target += PS;
	}
#endif

	//dump heap section
	offset = PS * (mcode_pages + mdata_pages);
	addr = (char*)(enclave_start_addr + offset);
	target = out + offset;

	memcpy(target, addr, mheap_pages*PS);
#if 0
	for(i = 0; i < mheap_pages; ++i)
	{
		for(j = 0; j < PS; ++j)
			target[j] = addr[j];
		addr += PS;
		target += PS;
	}
#endif

	//dump stack section
	offset = PS * (mcode_pages + mdata_pages + mheap_pages);
	addr = (char*)(enclave_start_addr + offset);
	target = out + offset;

	memcpy(target, addr, mstack_pages*PS);

#if 0
	for(i = 0; i < mstack_pages; ++i)
	{
		for(j = 0; j < PS; ++j)
			target[j] = addr[j];
		addr += PS;
		target += PS;
	}
#endif
	
	skip_tcs = 0;
	//dump thread section
	offset = PS * (mcode_pages + mdata_pages + mheap_pages + mstack_pages);
	addr = (char*)(enclave_start_addr + offset);
	target = out + offset;
	for(i = 0; i < mthread_pages; ++i)
	{
		if(skip_tcs % 3 != 0)
		{
			memcpy(target, addr, PS);
			//for(j = 0; j < PS; ++j)
			//	target[j] = addr[j];
		}
		else
		{
			for(j = 0; j < PS; ++j)
				target[j] = '\0';
		}
		addr += PS;
		target += PS;
		skip_tcs += 1;
	}
}
#endif
