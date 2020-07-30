#ifndef __VARS_H_
#define __VARS_H_

//global
//defined in the linker script
extern unsigned long enclave_start;
extern unsigned long enclave_end;
extern unsigned long heap_start;
extern unsigned long heap_end;

#define MAX_FD 8
#define UNTRACED_FD 4
#define MAX_PATH_LEN 48

#define MALLOC_AREA_SIZE   0x8000000    // 128 MB
#define MMAP_AREA_SIZE     0x20000000   // 512 MB
//#define N_THREADS 2

struct fd_info {
    char path[MAX_PATH_LEN];
    long cursor;
    int flags;
    int mode;
};

//enclave thread local storage
struct enclave_tls {
	struct enclave_tls *self; // point to itself
	unsigned etid; // enclave thread ID
	int _errno;
	unsigned long _register_frame_start;
	unsigned long _register_frame_end;
	unsigned long _outside_stack;
	unsigned long _outside_buffer;
	unsigned long _previous_stack;
	unsigned long _pthread_id;

	unsigned long _outside_fs;
    unsigned long _outside_tramp;
    struct fd_info _open_fds[MAX_FD];
};


//TLS varible definition

//already use the following two lines to substitude the definition in musl libc
//extern int *__tls_errno_location(void);
//#define errno (*__tls_errno_location()); 

extern unsigned long *__tls_context_start(void);
#define register_frame_start (*__tls_context_start())

extern unsigned long *__tls_context_end(void);
#define register_frame_end (*__tls_context_end())

extern unsigned long *__tls_outside_stack(void);
#define outside_stack (*__tls_outside_stack())

extern unsigned long *__tls_outside_buffer(void);
#define outside_buffer (*__tls_outside_buffer())

extern unsigned long *__tls_previous_stack(void);
#define previous_stack (*__tls_previous_stack())

extern int init_fd(int fd, char *path, int flag, int mode);
//extern int increase_fd_cursor(int fd, unsigned long size);
extern int set_fd_cursor(int fd);
extern int set_fd_cursor_at(int fd, long offset);
extern int close_fd(int fd);

#endif
