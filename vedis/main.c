#include <stdio.h>  /* puts() */
#include <stdlib.h> /* exit() */
#include <unistd.h> /* usleep() */
/* Make sure this header file is available.*/
#include "vedis.h"
/* Forward declaration: Data consumer callback */
static int DataConsumerCallback(const void *pData,unsigned int nDatalen,void *pUserData /* Unused */);
/*
 * Maximum records to be inserted in our datastore.
 */
#define MAX_RECORDS 10000000
#define INSERT_RATIO 5

char *aarch64_fn = "./enclave/vedis_aarch64";
char *x86_64_fn  = "./enclave/vedis_x86_64";

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
	
	printf("Starting operating on %lu random records...\n", records_num);
	
	/* Start the random insertions */
	for( i = 0 ; i < records_num; ++i ){
#ifndef NO_MIGRATION
		if (i % 100000 == 0)
			check_migrate(1, 0);
#endif
		
		/* Genearte the random key first */
		vedis_util_random_string(pStore,zKey,sizeof(zKey));
        randnum = vedis_util_random_num(pStore) % 100;

		/* Perform the insertion */
        //if (randnum < 0) { printf("???? < 0\n"); return 1; }
        //if (i % 10000 == 0) { printf("randnum: %d\n", randnum); }
        if (randnum < insert_ratio) {
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

			/* time(&tt); pTm = localtime(&tt); ... */
			vedis_kv_store_fmt(pStore,"sentinel",-1,"I'm a sentinel record inserted on %d:%d:%d\n",14,15,18); /* Dummy time */
		}
	}
	
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
