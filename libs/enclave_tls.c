#include <string.h>  // strlen
#include <sys/types.h> // SEEK_CUR
#include <unistd.h>  // lseek
#include "vars.h"
#include "pthread.h"

static inline struct enclave_tls *__tls_self()
{
	struct enclave_tls *self;
#ifdef __x86_64
	__asm__ __volatile__("mov %%fs:0, %0" : "=r" (self));
#else
	__asm__ __volatile__("mrs %0, tpidr_el0" : "=r"(self));
#endif
	return self;                                          
}                                                         

unsigned *__tls_etid(void)
{
    return &(__tls_self()->etid);
}

int *__tls_errno_location(void)
{
	return &(__tls_self()->_errno);
}

unsigned long *__tls_context_start(void)
{
	return &(__tls_self()->_register_frame_start);
}

unsigned long *__tls_context_end(void)
{
	return &(__tls_self()->_register_frame_end);
}

unsigned long *__tls_outside_stack(void)
{
	return &(__tls_self()->_outside_stack);
}

unsigned long *__tls_outside_buffer(void)
{
	return &(__tls_self()->_outside_buffer);
}

unsigned long *__tls_previous_stack(void)
{
	return &(__tls_self()->_previous_stack);
}

int init_fd(int fd, char *path, int flags, int mode) {
    fd -= UNTRACED_FD;
    if (fd < 0 || fd >= MAX_FD)
        return -1;
    size_t path_len = strlen(path);
    if (path_len > MAX_PATH_LEN - 1)
        return -1;

    struct fd_info *f = &__tls_self()->_open_fds[fd];
    memcpy(f->path, path, path_len);
    f->path[path_len] = 0;
    f->cursor = 0;
    f->flags = flags;
    f->mode = mode;

    return 0;
}

//int increase_fd_cursor(int fd, unsigned long size) {
//    fd -= UNTRACED_FD;
//    if (fd < 0 || fd >= MAX_FD)
//        return -1;
//    __tls_self()->_open_fds[fd].cursor += size;
//    return 0;
//}

int set_fd_cursor(int fd) {
    fd -= UNTRACED_FD;
    if (fd < 0 || fd >= MAX_FD)
        return -1;
    /* __tls_self()->_open_fds[fd].cursor = */ lseek(fd + UNTRACED_FD, 0, SEEK_CUR);
    return 0;
}

int set_fd_cursor_at(int fd, long offset) {
    fd -= UNTRACED_FD;
    if (fd < 0 || fd >= MAX_FD)
        return -1;
    __tls_self()->_open_fds[fd].cursor = offset;
    return 0;
}

int close_fd(int fd) {
    //printf("close_fd %d, change %p\n", fd, __tls_self()->_open_fds[fd-UNTRACED_FD].path[0]);
    fd -= UNTRACED_FD;
    if (fd < 0 || fd >= MAX_FD)
        return -1;
    __tls_self()->_open_fds[fd].path[0] = 0;
    return 0;
}

pthread_t pthread_self(void)
{
	return (pthread_t)__tls_self()->_pthread_id;
}
