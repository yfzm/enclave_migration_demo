//TODO: some many "if" will decrease the performance.
//use switch or if/else

//TODO check:
//1. whether fcntl needs extra copy. 
//2. whether ioctl needs extra copy. 
//3. after ocall_syscall, checking return value before copying results

//default: musl-libc/include
#include "sys/uio.h" 
#include "bits/ioctl.h"
//#include "sys/ioctl.h"
#include "stdio.h"
#include "stdbool.h"
#include "time.h"
#include "string.h"
#include "bits/syscall.h" //define all the syscall number
//#include "sys/syscall.h"
#include "sys/stat.h"
#include "string.h"
#include "errno.h"
#include "sys/resource.h"
#include "sys/socket.h"
#include "sys/epoll.h"
#include "fcntl.h"
#include "sys/file.h"
#include "assert.h"

// $(pwd)/include
#include "vars.h"

#define MMAP_RECLAIM
#define MMAP_DEBUG

extern unsigned long enclave_start;

unsigned long __brk = 0 ; //used in migration thread
unsigned long __init_brk = 0; //used in migration thread

unsigned long __cur_mmap = 0;

#ifdef MMAP_RECLAIM
unsigned long __pending_free_addr = 0;
unsigned long __pending_free_size = 0;
#endif

#ifdef MMAP_DEBUG
# define yfzm_printf(...) printf(__VA_ARGS__)
#else
# define yfzm_printf(...)
#endif

//simple spin lock
/*
volatile static int lockflag;
//lock
static void lock()
{
	while(__sync_bool_compare_and_swap(&lockflag, 0, 1) == false);
}

static void unlock()
{
	lockflag = 0;
}
*/

//do not need copying args
//SYS_listen, SYS_socket, SYS_close, SYS_epoll_create1


//declaration
void ocall_syscall();

//For debugging
void ocall_debug(long a1)
{
	unsigned long *ptr;

	ptr = (unsigned long*)outside_buffer;
	*ptr = 7;
	*(ptr+1) = a1;
	ocall_syscall();
}

void ocall_senddata()
{
    unsigned long *ptr;
    ptr = (unsigned long *)outside_buffer;
    *ptr = 9;
    ocall_syscall();
}

long ocall_syscall0(long n)
{
	long ret;
	unsigned long *ptr;

	ptr = (unsigned long*)outside_buffer;
	*ptr = 0;
	*(ptr+1) = n;
	ocall_syscall();
	ret = *ptr;
	return ret;	
}

extern unsigned long fake_heap;
long ocall_syscall1(long n, long a1)
{
	long ret;
	unsigned long *ptr;
		
	void *ptr_out;

	/*
	//unsafe heap: when enclave size is small
	if(n == SYS_brk) // 12
	{
		if(a1 == 0) //ask the start of heap
		{
			return fake_heap;
		}
		else if((unsigned long)a1 <= (fake_heap + 128*1024*1024))
		{
			return a1;
		}
		else
		{
			ocall_debug(0xdeadbeef);
			return -ENOMEM; //emulate the syscall
		}
	}
	*/

	//use internal heap: if running gcc, we need more internal heap so that the 
	//size of enclave should at least be 0x8000.
	if(n == SYS_brk) // 12
	{
        printf("[SYS_brk] a1=%p, __brk=%p(%p)\n", a1, __brk, &__brk);  //yfzm
		if(a1 == 0) //ask the start of heap
		{
			__init_brk = (long)&heap_start;
			return (long)&heap_start;
		}
		else if((unsigned long)a1 <= (((unsigned long)&heap_start) + MALLOC_AREA_SIZE))
		{
			__brk = a1;
			return a1; //use 128M inside the heap region for malloc (other for mmap)
		}
		else
		{
			ocall_debug(0xdead);
			return -ENOMEM;
		}
	}


	ptr = (unsigned long*)outside_buffer;
	*ptr = 1;
	*(ptr+1) = n;
	*(ptr+2) = a1;

#ifdef SYS_pipe
	if(n == SYS_pipe) //22
	{
		ptr_out = (void*)outside_buffer + 0x1000;
		*(ptr+2) = (unsigned long)ptr_out;
		memcpy(ptr_out, (void*)a1, sizeof(int)*2);
	}
#endif

	if(n == SYS_set_tid_address) //set_tid_address
	{
		ptr_out = (void*)outside_buffer + 0x1000;
		*(ptr+2) = (unsigned long)ptr_out;
	}

	ocall_syscall();

    if (n == SYS_close) {
        ret = *ptr;
        //printf("[SYS_close] return %d, fd %d\n", ret, a1);
        if (ret == 0) {
            //printf("[sys_close] %d\n", a1);
            close_fd(a1);
        }
        return ret;
    }

#ifdef SYS_pipe
	if(n == SYS_pipe) //22
	{
		ptr_out = (void*)outside_buffer + 0x1000;
		memcpy((void*)a1, ptr_out, sizeof(int)*2);
	}
#endif

	if(n == SYS_set_tid_address)
	{
		memcpy((void*)a1, ptr_out, 8);
	}

	ret = *ptr;
	return ret;	
}

long ocall_syscall2(long n, long a1, long a2)
{
	long ret;
	unsigned long *ptr;

	void *ptr_out;
	void *ptr_in;
	int len;

    if (n == SYS_munmap)
    {
        yfzm_printf("[SYS_munmap] addr %p, len 0x%lx\n", a1, a2);
		if ((unsigned long)a1 > 0xffff00000000) {
			printf("Error!\n");
			assert(0);
		}
        if ((a1 >= (unsigned long)&enclave_start) && (a1 < (unsigned long)&enclave_start + 0x40000000)) {

            if ((a1 >= (unsigned long)(&heap_start) + MALLOC_AREA_SIZE) &&
                    (a1 < (unsigned long)(&heap_start) + MALLOC_AREA_SIZE + MMAP_AREA_SIZE)) {
#ifdef MMAP_RECLAIM
                if (a1 + a2 == __cur_mmap) {
                    if (__pending_free_addr + __pending_free_size == a1) {
						__pending_free_addr = 0;
                        __pending_free_size = 0;
                        __cur_mmap = __pending_free_addr;
                        yfzm_printf("  [munmap] case 1: reclaim pending\n");
                    } else {
                        __cur_mmap = a1;
                        yfzm_printf("  [munmap] case 2: reclaim normal\n");
                    }
                } else {
                    if (__pending_free_addr + __pending_free_size == a1) {
                        __pending_free_size += a2;
                        yfzm_printf("  [munmap] case 3: merge free below\n");
                    } else if (__pending_free_addr == a1 + a2) {
                        __pending_free_addr = a1;
                        __pending_free_size += a2;
                        yfzm_printf("  [munmap] case 4: merge free above\n");
                    } else {
                        __pending_free_addr = a1;
                        __pending_free_size = a2;
                        yfzm_printf("  [munmap] case 5: record new\n");
                    }
                }
#else
                yfzm_printf("[SYS_munmap] skip enclave mmap area\n");
#endif
                return 0;
            }
            printf("  [munmap] Error: unmap no-mmap area in enclave\n");
            exit(1);
        } else {
            yfzm_printf("  [munmap] skip outside mmap area\n");
        }

    }

	ptr = (unsigned long*)outside_buffer;
	*ptr = 2;
	*(ptr+1) = n;
	*(ptr+2) = a1;
	*(ptr+3) = a2;

	if(n == SYS_nanosleep)
	{
		ptr_in = (void*)a1;
		ptr_out = (void*)outside_buffer + 0x1000;
		*(ptr+2) = (unsigned long)ptr_out;
		memcpy(ptr_out, ptr_in, sizeof(struct timespec));

		if(a2 != 0)
		{
			ptr_in = (void*)a2;
			ptr_out = (void*)outside_buffer + 0x1000 + sizeof(struct timespec);
			*(ptr+3) = (unsigned long)ptr_out;
			memcpy(ptr_out, ptr_in, sizeof(struct timespec));
		}
	}

	if(n == SYS_clock_gettime) // 228
	{
		ptr_out = (void*)outside_buffer + 0x1000;
		*(ptr+3) = (unsigned long)ptr_out;
	}

//	if(n == SYS_arch_prctl) //158
//	{
//		ocall_debug(0xdeadbeef);
//		// asm("rdtsc");
//		/*
//		if(a1 == ARCH_GET_FS)
//		{
//			*(unsigned long*)a2 = (unsigned long)pthread_self();
//			return 0;
//		}
//		else //a1 == ARCH_GET_GS ARCH_SET_FS or ARCH_SET_GS
//		{
//			ocall_debug(0xdeadbeef);
//			asm("rdtsc");
//		}
//		*/
//	}

#ifdef SYS_stat
	if(n == SYS_stat) // 4
	{
		len = strlen((char*)a1) + 1;
		ptr_in = (char*)a1;
		ptr_out = (char*)outside_buffer + 0x1000;
		*(ptr+2) = (unsigned long)ptr_out;
		memcpy(ptr_out, ptr_in, len);			

		ptr_in = (char*)a2;
		ptr_out = (char*)outside_buffer + 0x1000 + len;
		*(ptr+3) = (unsigned long)ptr_out;
	}
#endif

	if(n == SYS_fstat)
	{
		ptr_out = (void*)outside_buffer + 0x1000;
		*(ptr+3) = (unsigned long)ptr_out;
	}

	ocall_syscall();

	if(n == SYS_nanosleep)
	{
		if(a2 != 0)
		{
			ptr_in = (void*)a2;
			ptr_out = (void*)outside_buffer + 0x1000 + sizeof(struct timespec);
			memcpy(ptr_in, ptr_out, sizeof(struct timespec));
		}
	}

#ifdef SYS_stat
	if(n == SYS_stat)
	{
		ptr_in = (char*)a2;
		ptr_out = (char*)outside_buffer + 0x1000 + len;

		memcpy(ptr_in, ptr_out, sizeof(struct stat));
	}
#endif

	if(n == SYS_fstat)
	{
		ptr_in = (char*)a2;
		ptr_out = (void*)outside_buffer + 0x1000;

		memcpy(ptr_in, ptr_out, sizeof(struct stat));
	}

	if(n == 228)
		memcpy((void*)a2, ptr_out, sizeof(struct timespec));

	ret = *ptr;
	//if(ret == 0) return ret;

	//errno is a positive number.
	errno = -ret; //set the errno
	//return -1; //From the manual: -1 means error, errno shows the reason.
	return ret;
}

// void check_fs()                    
// {                                  
// 	unsigned long val;             

// 	asm volatile(                  
// 			"mov %%fs:0, %%rax\n\t"
// 			"mov %%rax, %0\n\t"    
// 			:"=m"(val)::           
// 			);                     

// 	if(val != *(unsigned long*)val)
// 	{                              
// 		while(1){}                 
// 	}                              

// 	if((val < 0x1bff6000) || (val > 0x1c000000))
// 	{
// 		while(1){}
// 	}
// }                                  


long ocall_syscall3(long n, long a1, long a2, long a3)
{	
	long ret;
	unsigned long *ptr;

	int i;
	int j;
	void *base1;
	void *base2;
	int offset;
	struct iovec* s_vec;
	struct iovec* t_vec;

	char *ptr_in;
	char *ptr_out;
	socklen_t len;

	ptr = (unsigned long*)outside_buffer;

	//check_fs();

	*ptr = 3;
	*(ptr+1) = n;
	*(ptr+2) = a1;
	*(ptr+3) = a2;
	*(ptr+4) = a3;

	if(n == SYS_fcntl)
	{
		/*
		The third arg depends on the second arg (cmd)
		F_DUPFD is 0            
		F_GETFD is 1            
		F_SETFD is 2  (int)      
		F_GETFL is 3  (void)
		F_SETFL is 4  (int)      
		F_GETLK is 5            
		F_SETLK is 6            
		F_SETLKW is 7           
		F_DUPFD_CLOEXEC is 1030 
		*/

		/*
		if(a2 == F_SETFL) // 3 or 4
		{
			ocall_debug(a3);
			if(a3 != 0)
			{
				ptr_in = (char*)a3;
				ptr_out = (char*)outside_buffer + 0x1000;	
				*(ptr+4) = (unsigned long)ptr_out;
				memcpy(ptr_out, ptr_in, sizeof(struct flock));
			}
		}
		*/
	}

	if(n == SYS_connect)
	{
		ptr_in = (char*)a2;
		ptr_out = (char*)outside_buffer + 0x1000;	
		*(ptr+3) = (unsigned long)ptr_out;
		memcpy(ptr_out, ptr_in, a3);	
	}

	if(n == SYS_bind)
	{
		ptr_out = (char*)outside_buffer + 0x1000;
		*(ptr+3) = (unsigned long)ptr_out;
		memcpy(ptr_out, (void*)a2, a3);
	}

	if(n == SYS_accept) //43
	{
		if(a2 != 0)
		{
			ptr_in = (char*)a3;
			ptr_out = (char*)outside_buffer + 0x1000;
			*(ptr+4) = (unsigned long)ptr_out;
			memcpy(ptr_out, ptr_in, sizeof(socklen_t));

			ptr_out = (char*)outside_buffer + 0x1000 + sizeof(socklen_t);
			*(ptr+3) = (unsigned long)ptr_out;
		}
	}

	if(n == SYS_write)
	{
		ptr_in = (char*)a2;
		ptr_out = (char*)outside_buffer + 0x1000;
		*(ptr+3) = (unsigned long)ptr_out;
		
		memcpy(ptr_out, ptr_in, a3);
	}

	if(n == SYS_read)
	{
		ptr_out = (char*)outside_buffer + 0x1000;
		*(ptr+3) = (unsigned long)ptr_out;
	}

	if(n == SYS_writev || n == SYS_readv) // 20, 19
	{
		base1 = (void*)outside_buffer + 0x1000;
		*(ptr+3) = (unsigned long)base1;

		base2 = base1 + sizeof(struct iovec) * a3;
		offset = 0;

		s_vec = (struct iovec*)a2;
		t_vec = (struct iovec*)base1;

		//copy args out of the enclave
		for(i = 0; i < a3; ++i)
		{
			t_vec->iov_base = base2 + offset;
			t_vec->iov_len = s_vec->iov_len;
			for(j = 0; j < t_vec->iov_len; ++j)
				*(char*)(t_vec->iov_base+j) = *(char*)(s_vec->iov_base+j);
			offset += t_vec->iov_len;
			s_vec += 1;
			t_vec += 1;
		}
	}

#ifdef SYS_open
	if(n == SYS_open) // 2
	{
		ptr_in = (char*)a1;
		ptr_out = (char*)outside_buffer + 0x1000;
		*(ptr+2) = (unsigned long)ptr_out;
	
		i = strlen(ptr_in) + 1; // include the str ending '\0'
		memcpy(ptr_out, ptr_in, i);
	}
#endif
    
	if(n == 16) // ioctl
	{
		ptr_out = (char*)outside_buffer + 0x1000;
		*(ptr+4) = (unsigned long)ptr_out;
	}

	ocall_syscall();

	if(n == SYS_fcntl)
	{
		/*
		if(a2 == F_GETFL || a2 == F_SETFL) // 3 or 4
		{
			if(a3 != 0)
			{
				ptr_in = (char*)a3;
				ptr_out = (char*)outside_buffer + 0x1000;	
				memcpy(ptr_in, ptr_out, sizeof(struct flock));
			}
		}
		*/
	}

	if(n == SYS_accept) //43
	{
		if(a2 != 0)
		{
			ptr_in = (char*)a3;
			ptr_out = (char*)outside_buffer + 0x1000;
			memcpy(ptr_in, ptr_out, sizeof(socklen_t));

			len = *(socklen_t *)ptr_out;
			ptr_in = (char*)a2;
			ptr_out = (char*)outside_buffer + 0x1000 + sizeof(socklen_t);
			memcpy(ptr_in, ptr_out, (size_t)len);
		}
	}
	
	if(n == 16 && a2 == 0x5413) //get the size of window
	{
		memcpy((void*)a3, ptr_out, sizeof(struct winsize));
	}

#ifdef SYS_open
    if (n == SYS_open) {
        ret = *ptr;
        if (ret >= 0)
            init_fd(ret, a1, a2, a3);
        return ret;
    }
#endif

	if(n == SYS_readv) // readv: copy data into enclave
	{
		base1 = (void*)outside_buffer + 0x1000;

		base2 = base1 + sizeof(struct iovec) * a3;

		s_vec = (struct iovec*)base1;
		t_vec = (struct iovec*)a2;

		//copy args out of the enclave
		for(i = 0; i < a3; ++i)
		{
			for(j = 0; j < t_vec->iov_len; ++j)
				*(char*)(t_vec->iov_base+j) = *(char*)(s_vec->iov_base+j);
			s_vec += 1;
			t_vec += 1;
		}

        set_fd_cursor(a1);
	}

	if(n == SYS_read)
	{
		ret = *ptr;
		if(ret > 0)
		{
			ptr_in = (char*)a2;
			ptr_out = (char*)outside_buffer + 0x1000;
			memcpy(ptr_in, ptr_out, a3);
            set_fd_cursor(a1);
            //increase_fd_cursor(a1, ret);
		}
		if(ret < 0)
		{
			//This shoule be done in musl-libc. Musl-libc's bug? Maybe because it is modified.
			errno = -ret; 			
			return -1;
		}
	}

    if (n == SYS_write)
    {
        ret = *ptr;
        if (ret > 0) {
            set_fd_cursor(a1);
        }
        return ret;
    }

    if (n == SYS_lseek)
    {
        ret = *ptr;
        set_fd_cursor_at(a1, ret);
        return ret;
    }

	ret = *ptr;
	return ret;	
}

long ocall_syscall4(long n, long a1, long a2, long a3, long a4)
{
	long ret;
	unsigned long *ptr;

    int i;
	char *ptr_in;
	char *ptr_out;

	//if(n == SYS_rt_sigaction || n == SYS_rt_sigprocmask)
		//return 0;

	ptr = (unsigned long*)outside_buffer;
	*ptr = 4;
	*(ptr+1) = n;
	*(ptr+2) = a1;
	*(ptr+3) = a2;
	*(ptr+4) = a3;
	*(ptr+5) = a4;

	if(n == SYS_futex)
	{
		if(a4 != 0)
		{
			ptr_in = (char*)a4;
			ptr_out = (char*)outside_buffer + 0x1000;
			*(ptr+5) = (unsigned long)ptr_out;
			memcpy(ptr_out, ptr_in, sizeof(struct timespec));
		}
	}

	if(n == SYS_rt_sigprocmask)
	{
		if(a2 != 0)
		{
			ptr_in = (char*)a2;
			ptr_out = (char*)outside_buffer + 0x1000;
			*(ptr+3) = (unsigned long)ptr_out;
			memcpy(ptr_out, ptr_in, a4);
		}

		if(a3 != 0)
		{
			ptr_in = (char*)a3;
			ptr_out = (char*)outside_buffer + 0x1000 + a4;
			*(ptr+4) = (unsigned long)ptr_out;
			memcpy(ptr_out, ptr_in, a4);
		}
	}

	if(n == SYS_prlimit64) //302
	{
		if(a3 != 0)
		{
			ptr_in = (char*)a3;
			ptr_out = (char*)outside_buffer + 0x1000;
			*(ptr+4) = (unsigned long)ptr_out;
			memcpy(ptr_out, ptr_in, sizeof(struct rlimit));
		}

		if(a4 != 0)
		{
			ptr_in = (char*)a4;
			ptr_out = (char*)outside_buffer + 0x1000 + sizeof(struct rlimit);
			*(ptr+5) = (unsigned long)ptr_out;
			memcpy(ptr_out, ptr_in, sizeof(struct rlimit));
		}
	}

	if(n == SYS_epoll_ctl)
	{
		ptr_in = (char*)a4;
		ptr_out = (char*)outside_buffer + 0x1000;
		*(ptr+5) = (unsigned long)ptr_out;	
		memcpy(ptr_out, ptr_in, sizeof(struct epoll_event));
	}

	if(n == SYS_openat)
	{
		ptr_in = (char*)a2;
		ptr_out = (char*)outside_buffer + 0x1000;
		*(ptr+3) = (unsigned long)ptr_out;
	
		i = strlen(ptr_in) + 1; // include the str ending '\0'
		memcpy(ptr_out, ptr_in, i);
	}

	ocall_syscall();

    if (n == SYS_openat) {
        ret = *ptr;
        init_fd(ret, (char *)a2, a3, a4);
        return ret;
    }

	if(n == SYS_epoll_ctl)
	{
		ptr_in = (char*)a4;
		ptr_out = (char*)outside_buffer + 0x1000;
		memcpy(ptr_in, ptr_out, sizeof(struct epoll_event));
	}

	if(n == SYS_rt_sigprocmask)
	{
		if(a2 != 0)
		{
			ptr_in = (char*)a2;
			ptr_out = (char*)outside_buffer + 0x1000;
			*(ptr+3) = (unsigned long)ptr_out;
			memcpy(ptr_in, ptr_out, a4);
		}

		if(a3 != 0)
		{
			ptr_in = (char*)a3;
			ptr_out = (char*)outside_buffer + 0x1000 + a4;
			*(ptr+4) = (unsigned long)ptr_out;
			memcpy(ptr_in, ptr_out, a4);
		}
	}

	if(n == SYS_prlimit64)
	{
		if(a3 != 0)
		{
			ptr_in = (char*)a3;
			ptr_out = (char*)outside_buffer + 0x1000;
			memcpy(ptr_in, ptr_out, sizeof(struct rlimit));
		}

		if(a4 != 0)
		{
			ptr_in = (char*)a4;
			ptr_out = (char*)outside_buffer + 0x1000 + sizeof(struct rlimit);
			memcpy(ptr_in, ptr_out, sizeof(struct rlimit));
		}
	}

	ret = *ptr;
	return ret;	
}

long ocall_syscall5(long n, long a1, long a2, long a3, long a4, long a5)
{
	long ret;
	unsigned long *ptr;

	ptr = (unsigned long*)outside_buffer;
	*ptr = 5;
	*(ptr+1) = n;
	*(ptr+2) = a1;
	*(ptr+3) = a2;
	*(ptr+4) = a3;
	*(ptr+5) = a4;
	*(ptr+6) = a5;
	ocall_syscall();
	ret = *ptr;
	return ret;	
}

long ocall_syscall6(long n, long a1, long a2, long a3, long a4, long a5, long a6)
{
	long ret;
	unsigned long *ptr;

    if (n == SYS_mmap)
    {
        yfzm_printf("[SYS_mmap] addr %p, len 0x%lx, prot %d, flags %d, fd %d, offset %lu\n", a1, a2, a3, a4, a5, a6);
        yfzm_printf("  __cur_mmap: %p\n", __cur_mmap);
        if (a1 == 0 && a5 == -1) {
            if (__cur_mmap == 0) {
                __cur_mmap = (unsigned long)(&heap_start) + MALLOC_AREA_SIZE;
            }
            if (__cur_mmap + a2 < (unsigned long)(&heap_start) + MALLOC_AREA_SIZE + MMAP_AREA_SIZE) {
#ifdef MMAP_RECLAIM
                if (a2 <= __pending_free_size) {
                    __pending_free_addr += a2;
                    __pending_free_size -= a2;
					yfzm_printf("  [use reclaimed] %p\n", __pending_free_addr - a2);
                    return __pending_free_addr - a2;
                }
#endif
                __cur_mmap += a2;
				yfzm_printf("  [use new] %p\n", __cur_mmap - a2);
                return __cur_mmap - a2;
            } else {
                printf("[SYS_mmap] Error: exceed mmap area!\n");
				assert(0);
                exit(1);
            }
        }
        //printf("[SYS_mmap] not supported yet!\nUse outer mmap...\n");
    }
	
	char *ptr_in;
	char *ptr_out;

	socklen_t len;

	struct msghdr *hdr_out;
	struct msghdr *hdr_in;
	struct iovec *s_vec;
	struct iovec *t_vec;
	int i;
	int j;

	ptr = (unsigned long*)outside_buffer;
	*ptr = 6;
	*(ptr+1) = n;
	*(ptr+2) = a1;
	*(ptr+3) = a2;
	*(ptr+4) = a3;
	*(ptr+5) = a4;
	*(ptr+6) = a5;
	*(ptr+7) = a6;

	if(n == SYS_accept4) //288
	{
		if(a2 != 0)
		{
			ptr_in = (char*)a3;
			ptr_out = (char*)outside_buffer + 0x1000;
			*(ptr+4) = (unsigned long)ptr_out;
			memcpy(ptr_out, ptr_in, sizeof(socklen_t));

			ptr_out = (char*)outside_buffer + 0x1000 + sizeof(socklen_t);
			*(ptr+3) = (unsigned long)ptr_out;
		}
	}

	if(n == SYS_getsockname)
	{
		ptr_in = (char*)a3;		
		ptr_out = (char*)outside_buffer + 0x1000;
		*(ptr+4) = (unsigned long)ptr_out;
		memcpy(ptr_out, ptr_in, sizeof(socklen_t));

		ptr_out = (char*)outside_buffer + 0x1000 + sizeof(socklen_t);
		*(ptr+3) = (unsigned long)ptr_out;
	}

	if(n == SYS_getsockopt)
	{
		if(a4 != 0)
		{
			ptr_in = (char*)a5; //socklen_t* (int *)
			ptr_out = (char*)outside_buffer + 0x1000;
			*(ptr+6) = (unsigned long)ptr_out;
			memcpy(ptr_out, ptr_in, sizeof(socklen_t));

			ptr_out = (char*)outside_buffer + 0x1000 + sizeof(socklen_t);
			*(ptr+5) = (unsigned long)ptr_out;
		}
	}

	if(n == SYS_setsockopt)
	{
		if(a4 != 0 && a5 != 0)
		{
			ptr_in = (char*)a4;
			ptr_out = (char*)outside_buffer + 0x1000;
			*(ptr+5) = (unsigned long)ptr_out;
			memcpy(ptr_out, ptr_in, a5);
		}
	}

	if(n == SYS_epoll_pwait)
	{
		if(a5 != 0)
		{
			ptr_out = (char*)outside_buffer + 0x1000;
			*(ptr+6) = (unsigned long)ptr_out;
			memcpy(ptr_out, (void*)a5, a6);
		}

		ptr_out = (char*)outside_buffer + 0x1000 + a6;
		*(ptr+3) = (unsigned long)ptr_out;
	}	

	if(n == SYS_getpeername)
	{
		len = *(socklen_t *)a3;
		ptr_out = (char*)outside_buffer + 0x1000;
		*(socklen_t *)ptr_out = len;
		*(ptr+4) = (unsigned long)ptr_out;

		ptr_out = (char*)outside_buffer + 0x1000 + sizeof(socklen_t);
		*(ptr+3) = (unsigned long)ptr_out;
	}

	if(n == SYS_socketpair) // 53
	{
		ptr_in = (char*)a4;
		ptr_out = (char*)outside_buffer + 0x1000;
		*(ptr+5) = (unsigned long)ptr_out;
		memcpy(ptr_out, ptr_in, sizeof(int)*2);
	}

	if(n == SYS_recvfrom)
	{
		ptr_out = (char*)outside_buffer + 0x1000;
		*(ptr+3) = (unsigned long)ptr_out;

		if(a5 != 0)
		{
			ptr_out = (char*)outside_buffer + 0x1000 + a3;
			*(ptr+7) = (unsigned long)ptr_out;
			memcpy(ptr_out, (void*)a6, sizeof(int));
			
			ptr_out = (char*)outside_buffer + 0x1000 + a3 + sizeof(int);
			*(ptr+6) = (unsigned long)ptr_out;
			memcpy(ptr_out, (void*)a5, a6);
		}
	}

	if(n == SYS_sendmsg)
	{
		ptr_in = (char*)a2;
		ptr_out = (char*)outside_buffer + 0x1000;
		*(ptr+3) = (unsigned long)ptr_out;
		hdr_out = (struct msghdr*)ptr_out;
		hdr_in = (struct msghdr*)ptr_in;

		memcpy(ptr_out, ptr_in, sizeof(struct msghdr));
		ptr_out += sizeof(struct msghdr);
		
		//deep copy	
		if(hdr_in->msg_name != NULL && hdr_in->msg_namelen != 0)
		{
			hdr_out->msg_name = ptr_out;
			memcpy(ptr_out, hdr_in->msg_name, hdr_in->msg_namelen);
			ptr_out += hdr_in->msg_namelen;
		}

		if(hdr_in->msg_control != NULL && hdr_in->msg_controllen != 0)
		{
			hdr_out->msg_control = ptr_out;
			memcpy(ptr_out, hdr_in->msg_control, hdr_in->msg_controllen);
			ptr_out += hdr_in->msg_controllen;
		}
		
		hdr_out->msg_iov = (struct iovec*)ptr_out;
		s_vec = hdr_in->msg_iov;
		t_vec = hdr_out->msg_iov;

		ptr_out += sizeof(struct iovec) * hdr_in->msg_iovlen;	
		for(i = 0; i < hdr_in->msg_iovlen; ++i)
		{
			t_vec->iov_base = ptr_out;
			t_vec->iov_len = s_vec->iov_len;

			for(j = 0; j < s_vec->iov_len; ++j)
				*(char*)(t_vec->iov_base+j) = *(char*)(s_vec->iov_base+j);

			ptr_out += s_vec->iov_len;
			s_vec += 1;
			t_vec += 1;
		}
	}

	if(n == SYS_sendto)
	{
		ptr_out = (char*)outside_buffer + 0x1000;
		*(ptr+3) = (unsigned long)ptr_out;
		memcpy(ptr_out, (void*)a2, a3);

		if(a5 != 0)
		{
			ptr_out = (char*)outside_buffer + 0x1000 + a3;
			*(ptr+6) = (unsigned long)ptr_out;
			memcpy(ptr_out, (void*)a5, a6);
		}
	}

	ocall_syscall();

	if(n == SYS_accept4) // 288
	{
		if(a2 != 0)
		{
			ptr_in = (char*)a3;
			ptr_out = (char*)outside_buffer + 0x1000;
			memcpy(ptr_in, ptr_out, sizeof(socklen_t));

			len = *(socklen_t *)ptr_out;
			ptr_in = (char*)a2;
			ptr_out = (char*)outside_buffer + 0x1000 + sizeof(socklen_t);
			memcpy(ptr_in, ptr_out, (size_t)len);
		}
	}

	if(n == SYS_getsockname)
	{
		ret = *ptr;
		if(ret == 0)
		{
			ptr_in = (char*)a3;	
			ptr_out = (char*)outside_buffer + 0x1000;
			memcpy(ptr_in, ptr_out, sizeof(socklen_t));

			len = *(socklen_t *)ptr_out;
			ptr_in = (char*)a2;
			ptr_out = (char*)outside_buffer + 0x1000 + sizeof(socklen_t);
			memcpy(ptr_in, ptr_out, (size_t)len);
		}
	}

	if(n == SYS_getsockopt)
	{
		ret = *ptr;
		if(ret == 0 && a4 != 0)
		{
			ptr_in = (char*)a5; //socklen_t* (int *)
			ptr_out = (char*)outside_buffer + 0x1000;
			memcpy(ptr_in, ptr_out, sizeof(socklen_t));
			
			len = *(socklen_t *)ptr_out;
			ptr_in = (char*)a4;	
			ptr_out = (char*)outside_buffer + 0x1000 + sizeof(socklen_t);
			memcpy(ptr_in, ptr_out, (size_t)len);
		}
	}

	if(n == SYS_epoll_pwait)
	{
		ret = *ptr;
		if(ret > 0)
		{
			ptr_out = (char*)outside_buffer + 0x1000 + a6;
			memcpy((void*)a2, ptr_out, sizeof(struct epoll_event)*ret);
		}
	}

	if(n == SYS_getpeername)
	{
		ret = *ptr;
		if(ret == 0)
		{
			ptr_out = (char*)outside_buffer + 0x1000;
			len = *(socklen_t *)ptr_out;
			*(socklen_t *)a3 = len;

			ptr_out = (char*)outside_buffer + 0x1000 + sizeof(socklen_t);
			memcpy((void*)a2, ptr_out, (size_t)len);
		}
	}

	if(n == SYS_socketpair) // 53
	{
		ptr_in = (char*)a4;
		ptr_out = (char*)outside_buffer + 0x1000;
		memcpy(ptr_in, ptr_out, sizeof(int)*2);
	}

	if(n == SYS_recvfrom)
	{
		ptr_out = (char*)outside_buffer + 0x1000;
		memcpy((void*)a2, ptr_out, a3);

		if(a5 != 0)
		{
			ptr_out = (char*)outside_buffer + 0x1000 + a3;
			memcpy((void*)a6, ptr_out, sizeof(int));
			
			ptr_out = (char*)outside_buffer + 0x1000 + a3 + sizeof(int);
			memcpy((void*)a5, ptr_out, (size_t)a6);
		}
	}

	ret = *ptr;
	return ret;	
}
