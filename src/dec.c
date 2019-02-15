/* Simple command line tool for snappy */
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>		// gettimeofday
#include "snappy.h"
#include "map.h"
#include "util.h"

#ifndef CACHELINE
#define CACHELINE 64
#endif

#ifndef TIMINGINFO
#define TIMINGINFO 1
#endif

int main(int ac, char **av)
{
	if(ac < 3 || ac >3) {
		printf("Parameter error! To decompress, please use:\n");
		printf("./exe compressedfile decompressedfile");
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

#ifdef TIMINGINFO
	struct timeval timer0, timer1, timer2;
	gettimeofday(&timer0,NULL);
#endif

	// allocate buffer for the output: obuff;
	size_t outlen;
	int err = snappy_uncompressed_length(ibuff,size,&outlen);
	obuff = malloc(outlen);
	if(obuff == NULL) {
		printf("memory allocation errori\n");
		exit(1);
	}

#ifdef TIMINGINFO
	gettimeofday(&timer1,NULL);
#endif
	// decompress from ibuff to obuff
	err = snappy_uncompress(ibuff,size,obuff);
	
#ifdef TIMINGINFO
	gettimeofday(&timer2,NULL);
	double runningtimeAll = (timer2.tv_sec-timer0.tv_sec)*1000000L+timer2.tv_usec-timer0.tv_usec;
	double runningtime = (timer2.tv_sec-timer1.tv_sec)*1000000L+timer2.tv_usec-timer1.tv_usec;
	printf("Decompression time is %ld microsec\n",(timer2.tv_sec-timer1.tv_sec)*1000000L+timer2.tv_usec-timer1.tv_usec);
	double throughputAll = (double)size/runningtimeAll/1.024/1.024;
	double throughput = (double)size/runningtime/1.024/1.024;
	printf("The throughput (with memory allocation) is:\t %f MB/s\n", throughputAll);
	printf("The throughput (without memory allocation) is:\t %f MB/s\n", throughput);
#endif

	// check whether error occurs
	if (err) {
		printf("decompression error: %d\n",err);
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
