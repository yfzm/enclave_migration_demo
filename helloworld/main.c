//musl-libc
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/stat.h>
#include <errno.h>
#include <syscall.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

//$(pwd)/include
//#include "vars.h"
#include "pthread.h"

extern int migration_flag;

char *aarch64_fn = "./enclave/enclave_aarch64";
char *x86_64_fn  = "./enclave/enclave_x86_64";

int main(int argc, char* argv[])
{
	if (argc > 1) {
		unsigned long size = atol(argv[1]) - 16;
		if (size > 0)
			printf("malloc %luMB memory at %p\n", size, malloc(size * 0x100000));
	}
#if 0
	int msg_cnt;
	msg_cnt = 0;
	while(1)
	{
		printf("This is the No.%d message from the app\n", ++msg_cnt);
		sleep(1);
	}
#else
    int msg_cnt = 0;
	printf("This is the No.%d message from the app\n", ++msg_cnt);
    while (1) {
#ifndef NO_MIGRATION
        check_migrate(1, 0);
#endif
		if (msg_cnt == 10) return 0; 
		printf("This is the No.%d message from the app\n", ++msg_cnt);
		sleep(1);
    }
#endif
	return 0;
}
