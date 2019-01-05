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
#include <sys/time.h>

#ifndef CACHELINE
#define CACHELINE 64
#endif

#ifndef MAX_NUM_OUTPUT
#define MAX_NUM_OUTPUT 128
#endif

#ifndef TIMINGINFO
#define TIMINGINFO 1
#endif

int main(int ac, char **av)
{
	if(ac < 3 || ac > 3) {
		printf("Parameter error! Please use:\n");
		printf("./exe input output\n");
		return -1;
	}

	// record the number of input files, 1 for exe, 1 for output file	
	const char * inputfile = av[1];
	const char * outputfile = av[2];

        int i=0;
	int outputNum;
	size_t totalFileSize = 0;
        size_t size[MAX_NUM_OUTPUT];
        size_t outlen[MAX_NUM_OUTPUT];
	char *ibuff[MAX_NUM_OUTPUT], *obuff[MAX_NUM_OUTPUT];

        char * outputfileGroup[MAX_NUM_OUTPUT];
	for(i=0; i<MAX_NUM_OUTPUT; i++) {
		const char* snp = ".txt";
		char NoOfFile[4];
		snprintf (NoOfFile, sizeof(NoOfFile), "%d", i);

		outputfileGroup[i] = malloc(strlen(outputfile)+3+4+1);	//outputfile001.txt \0
		strcpy(outputfileGroup[i],outputfile);
		strcat(outputfileGroup[i], NoOfFile);
		strcat(outputfileGroup[i],snp);
	}

	// open input file
	FILE * rFile = fopen(inputfile,"rb");
	if (rFile == NULL) {
		printf("File open error: %s!\n", inputfile);
		exit(1);
	}

	// read the meta data: the number of compressed files in the combined file
	long rv = fread(&outputNum, sizeof(outputNum), 1, rFile);
	if(rv!=1) {
		printf("file reading error for the output file number!\n");
		exit(1);
	}

	// read the meta data: the size of the compressed files in the combined file
	rv = fread(size, sizeof(size_t), outputNum, rFile);
	if(rv!=outputNum) {
		printf("file reading error for the compressed size of the output files!\n");
		exit(1);
	}

	// read the file data into memory (buffer): ibuff
	for(i=0; i<outputNum; i++) {
		// allocate ibuff[i] for each file
		ibuff[i] = malloc(size[i]);	
		if(ibuff[i] == NULL) {
			printf("memory allocation error for intput file buffers!\n");
			exit(1);
		}
		// read the files into ibuff[i]
		rv = fread(ibuff[i], sizeof(char), size[i], rFile);
		if(rv!=size[i]) {
			printf("file reading error for the file data!\n");	
		}
		totalFileSize += size[i];
	}	

#ifdef TIMINGINFO
	struct timeval timer0, timer1, timer2;
	gettimeofday(&timer0,NULL);
#endif

	// allocate memory for output files: obuff
	for(i=0; i<outputNum; i++) {
		// calculate the sizes of the output
		int err = snappy_uncompressed_length(ibuff[i],size[i],&outlen[i]);
		if(!err) {
			printf("Snappy file length error!\n");
		}
		obuff[i] = malloc(outlen[i]);
		if(obuff[i] == NULL) {
			printf("memory allocation error for output file buffers!\n");
			exit(1);
		}

	}

#ifdef TIMINGINFO
	gettimeofday(&timer1,NULL);
#endif

	// decompress from ibuff to obuff
	for(i=0; i<outputNum; i++) {
		int err = snappy_uncompress(ibuff[i],size[i],obuff[i]);

		// check whether error occurs
		if (err) {
			printf("decompression error: %d\n",err);
			exit(1);
		}
	}

#ifdef TIMINGINFO
	gettimeofday(&timer2,NULL);
	double runningtimeAll = (timer2.tv_sec-timer0.tv_sec)*1000000L+timer2.tv_usec-timer0.tv_usec;
	double runningtime = (timer2.tv_sec-timer1.tv_sec)*1000000L+timer2.tv_usec-timer1.tv_usec;
	printf("Decompression time is %ld microsec\n",(timer2.tv_sec-timer1.tv_sec)*1000000L+timer2.tv_usec-timer1.tv_usec);
	double throughputAll = (double)totalFileSize/runningtimeAll/1.024/1.024;
	double throughput = (double)totalFileSize/runningtime/1.024/1.024;
	printf("The throughput (with memory allocation) is:\t %f MB/s\n", throughputAll);
	printf("The throughput (without memory allocation) is:\t %f MB/s\n", throughput);
#endif

	// write the output file
	for(i=0; i<outputNum; i++) {
		FILE * wFile;
	        wFile=fopen(outputfileGroup[i],"wb");
       		rv = fwrite(obuff[i],sizeof(char),outlen[i],wFile);
		if(rv!=outlen[i]) {
			printf("file writing error for file NO.%d\n", i);
			exit(1);
		}

		// remember to close the file
		fclose(wFile);
	}

	// free the ibuff and obuff
	for(i=0; i<outputNum; i++) {
		free(ibuff[i]);
		free(obuff[i]);	
	}	
	
	// remember to close the file
	fclose(rFile);

	return 0;
}
