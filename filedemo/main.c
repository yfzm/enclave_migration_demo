//musl-libc
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <syscall.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

char *aarch64_fn = "./enclave/enclave_aarch64";
char *x86_64_fn  = "./enclave/enclave_x86_64";

char *filename = "./txt";

int main(int argc, char* argv[])
{
#if 1
    int fd = open(filename, O_RDWR);
    //printf("[check] %p: %p\n", 0x7ffd2048, *(void **)(0x7ffd2048));
    //printf("[check] %p: %p\n", 0x7ffd2078, *(void **)(0x7ffd2078));
    unsigned char c;
	while(1)
	{
        check_migrate(0, 0);
        if (read(fd, &c, 1) == 0) break;
        printf("GET: %c\n", c);
        //printf("[check] %p: %p\n", 0x7ffd2078, *(void **)(0x7ffd2078));
		sleep(1);
	}
    
    close(fd);
    printf("exit...\n");
#else
    int msg_cnt = 0;
	printf("This is the No.%d message from the app\n", ++msg_cnt);
    while (1) {
        check_migrate(0, 0);
		printf("This is the No.%d message from the app\n", ++msg_cnt);
		sleep(1);
    }
#endif
	return 0;
}
