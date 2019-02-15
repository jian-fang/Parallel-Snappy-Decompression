/* Simple command line tool for snappy */
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "snappy.h"
#include "map.h"
#include "util.h"

#ifndef CACHELINE
#define CACHELINE 64
#endif

int main(int ac, char **av)
{
	if(ac < 3 || ac >3) {
		printf("Parameter error! To compress, please use:\n");
		printf("./exe inputfile compressedfile");
		return -1;
	}
	
	const char* inputfile = av[1];
	const char* outputfile = av[2];

	// read the inputfile to the memory buffer: ibuff;
	FILE * rFile;
       	rFile=fopen(inputfile,"rb");
	if (rFile == NULL) {
		printf("File open error: %s!\n",inputfile);
		exit(1);
	}

	// get the file size
	fseek(rFile, 0, SEEK_END);
	size_t size = ftell(rFile);
	rewind(rFile);
	printf("the file size is %ld\n",size);

	char *ibuff = NULL, *obuff = NULL;
	ibuff = malloc(size);
	if(ibuff == NULL) {
		printf("memory allocation error\n");
		exit(1);
	}
	long rv = fread(ibuff,sizeof(char),size,rFile);
	if(rv!=size) {
		printf("file reading error\n");
		exit(1);
	}

	// allocate buffer for the output: obuff;
	size_t outlen = snappy_max_compressed_length(size);
	obuff = malloc(outlen);
	if(obuff == NULL) {
		printf("memory allocation errori\n");
		exit(1);
	}

	// compress from ibuff to obuff
	struct snappy_env env;
	snappy_init_env(&env);
	int err = snappy_compress(&env,ibuff,size,obuff,&outlen);
	snappy_free_env(&env);
	
	// check whether error occurs
	if (err) {
		printf("compression error: %d\n",err);
		exit(1);
	}

	// write the output file
	FILE * wFile;
        wFile=fopen(outputfile,"wb");
       	rv = fwrite(obuff,sizeof(char),outlen,wFile);
	if(rv!=outlen) {
		printf("file writing error\n");
		exit(1);
	}

	fclose(rFile);
	fclose(wFile);

	// free the memory allocation
	free(ibuff);
	free(obuff);	

	return err;
}
