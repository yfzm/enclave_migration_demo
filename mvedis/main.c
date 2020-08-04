#include <stdio.h>  /* puts() */
#include <stdlib.h> /* exit() */
#include <unistd.h> /* usleep() */
#include <string.h> /* memset() */
/* Make sure this header file is available.*/
#include "vedis.h"
/* Forward declaration: Data consumer callback */
static int DataConsumerCallback(const void *pData,unsigned int nDatalen,void *pUserData /* Unused */);
/*
 * Maximum records to be inserted in our datastore.
 */
#define MAX_RECORDS 2000000
#define INSERT_RATIO 5

char *aarch64_fn = "./enclave/enclave_aarch64";
char *x86_64_fn  = "./enclave/enclave_x86_64";

struct thread_data {
	vedis *pStore;
	unsigned long records_num;
	int tid;
	int insert_ratio;
};

void *test(void *data) {
	printf("debug line %d\n", __LINE__);
    unsigned int randnum;
	char zKey[14];               /* Random generated key */
	char zData[32];              /* Dummy data */
    vedis_int64 zSize;
	int i,rc;

	printf("debug line %d\n", __LINE__);
	struct thread_data *td = (struct thread_data *)data;
	vedis *pStore = td->pStore;

	printf("debug line %d\n", __LINE__);
	/* Start the random insertions */
	for( i = 0 ; i < td->records_num; ++i ){
		if (i % 100000 == 0)
			check_migrate(1, 0);
		/* Genearte the random key first */
//	printf("debug line %d, pStore: %p\n", __LINE__, pStore);
		vedis_util_random_string(pStore,zKey,sizeof(zKey));
//	printf("debug line %d\n", __LINE__);
        randnum = vedis_util_random_num(pStore) % 100;

//	printf("debug line %d\n", __LINE__);
		/* Perform the insertion */
        //if (randnum < 0) { printf("???? < 0\n"); return 1; }
        //if (i % 10000 == 0) { printf("randnum: %d\n", randnum); }
        if (randnum < td->insert_ratio) {
		    rc = vedis_kv_store(pStore,zKey,sizeof(zKey),zData,sizeof(zData));
        } else {
            zSize = sizeof(zData);
            rc = vedis_kv_fetch(pStore,zKey,sizeof(zKey),zData,&zSize);
        }
		if( rc != VEDIS_OK && rc != VEDIS_NOTFOUND ){
			/* Something goes wrong */
			break;
		}

		if( i == 79125 ){
			/* Insert a sentinel record */
	printf("debug line %d\n", __LINE__);

			/* time(&tt); pTm = localtime(&tt); ... */
			vedis_kv_store_fmt(pStore,"sentinel",-1,"I'm a sentinel record inserted by thread %d\n", td->tid);
		}
	}

	printf("debug line %d\n", __LINE__);
	/* If we are OK, then manually commit the transaction */
	if( rc == VEDIS_OK ){
		/* 
		 * In fact, a call to vedis_commit() is not necessary since Vedis
		 * will automatically commit the transaction during a call to vedis_close().
		 */
		rc = vedis_commit(pStore);
		if( rc != VEDIS_OK ){
			/* Rollback the transaction */
			rc = vedis_rollback(pStore);
		}
	}
	
	printf("debug line %d\n", __LINE__);
	if( rc != VEDIS_OK && rc != VEDIS_NOTFOUND){
		/* Something goes wrong, extract the datastore error log and exit */
		printf("Unkown error in line %d, rc=%d\n", __LINE__, rc);
		exit(1);
	}
	puts("Done...Fetching the 'sentinel' record: ");

	/* Fetch the sentinel */
	rc = vedis_kv_fetch_callback(pStore,"sentinel",-1,DataConsumerCallback,0);
	if( rc != VEDIS_OK ){
		/* Can't happen */
		printf("Sentinel record not found\n");
		exit(1);
	}
}

int main(int argc,char *argv[])
{
	vedis *pStore;                /* Database handle */
	char zKey[14];               /* Random generated key */
	char zData[32];              /* Dummy data */
    vedis_int64 zSize;
	int i,rc;
    unsigned long records_num = MAX_RECORDS;
    int insert_ratio = INSERT_RATIO;
    unsigned int randnum;

	if (argc > 1)
		records_num = atol(argv[1]);
	if (argc > 2)
		insert_ratio = atoi(argv[2]);

	/* Open our datastore */
	//rc = vedis_open(&pStore,argc > 1 ? argv[1] /* On-disk DB */ : ":mem:" /* In-mem DB */);
	rc = vedis_open(&pStore,":mem:");
	if( rc != VEDIS_OK ){
		printf("Out of memory");
		exit(1);
	}
	
	//printf("Starting operating on %lu random records...\n", records_num);
	
	int N = 1;
	struct thread_data *data = (struct thread_data *)malloc(N * sizeof(struct thread_data));
	pthread_t *threads = (pthread_t *)malloc(N * sizeof(pthread_t));
	memset(data, 0, N * sizeof(struct thread_data));
	memset(threads, 0, N * sizeof(pthread_t));

	for (i = 0; i < N; i++) {
		printf("Creating thread %d\n", i);
		data[i].pStore = pStore;
		data[i].records_num = records_num;
		data[i].tid = i;
		data[i].insert_ratio = insert_ratio;
		if (pthread_create(&threads[i], NULL, test, (void *)(&data[i])) != 0) {
			fprintf(stderr, "Error creating thread %d\n", i);
			exit(1);
		}
	}

	for (i = 0; i < N; i++) {
		if (pthread_join(threads[i], NULL) != 0) {
			fprintf(stderr, "Error waiting for thread %d completion\n", i);
			exit(1);
		}
	}

	/* All done, close our datastore */
	vedis_close(pStore);
	return 0;
}

#include <unistd.h>
/*
 * The following define is used by the UNIX build process and have
 * no particular meaning on windows.
 */
#ifndef STDOUT_FILENO
#define STDOUT_FILENO	1
#endif
/*
 * Data consumer callback [vedis_kv_fetch_callback(), vedis_kv_cursor_key_callback(), etc.).
 * 
 * Rather than allocating a static or dynamic buffer (Inefficient scenario for large data).
 * The caller simply need to supply a consumer callback which is responsible of consuming
 * the record data perhaps redirecting it (i.e. Record data) to its standard output (STDOUT),
 * disk file, connected peer and so forth.
 * Depending on how large the extracted data, the callback may be invoked more than once.
 */
static int DataConsumerCallback(const void *pData,unsigned int nDatalen,void *pUserData /* Unused */)
{
	ssize_t nWr;
	nWr = write(STDOUT_FILENO,pData,nDatalen);
	if( nWr < 0 ){
		/* Abort processing */
		return VEDIS_ABORT;
	}
	/* All done, data was redirected to STDOUT */
	return VEDIS_OK;
}
