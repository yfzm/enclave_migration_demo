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
    //migration_flag = 1;
    // migrate(1, 0, 0);
    while (1) {
        check_migrate(0, 0);
		printf("This is the No.%d message from the app\n", ++msg_cnt);
		sleep(1);
    }
    //write(1, "Hello World!\n", 13);
#endif
	return 0;
}
