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

char *aarch64_fn = "./enclave/enclave_aarch64";
char *x86_64_fn  = "./enclave/enclave_x86_64";

void *work(void *data) {
    int msg_cnt = 0;
    while (1) {
        check_migrate(1, 0);
		printf("This is the No.%d message from new thread\n", ++msg_cnt);
		sleep(1);
    }
}

int main(int argc, char* argv[])
{
    pthread_t t;

    pthread_create(&t, NULL, work, NULL);
    int msg_cnt = 0;
    while (1) {
        check_migrate(1, 0);
		printf("This is the No.%d message from main thread\n", ++msg_cnt);
		sleep(1);
    }
    //write(1, "Hello World!\n", 13);
	return 0;
}
