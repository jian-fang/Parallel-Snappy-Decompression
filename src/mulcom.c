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

#ifndef MAX_NUM_INPUT
#define MAX_NUM_INPUT 256
#endif

int main(int ac, char **av)
{
	if(ac < 3) {
                printf("Parameter error! Please use:\n");
                printf("./exe input1 input2 input3 ... output\n");
                return -1;
        }

        // record the number of input files, 1 for exe, 1 for output file       
        int i=0;
        const int inputNum = ac-2;
        char * inputfileGroup[MAX_NUM_INPUT];
        size_t size[MAX_NUM_INPUT];
        size_t outlen[MAX_NUM_INPUT];
	char *ibuff[MAX_NUM_INPUT], *obuff[MAX_NUM_INPUT];

	// get the parameters
        for(i=0; i<inputNum; i++) {
                inputfileGroup[i] = av[1+i];
        }
        const char* outputfile = av[ac-1];

        // read the inputfile to the memory buffer: ibuff;
        FILE * rFile[MAX_NUM_INPUT];
        for(i=0; i<inputNum; i++) {
                //open file
                rFile[i]=fopen(inputfileGroup[i],"rb");
                if (rFile[i] == NULL) {
                        printf("File open error: %s!\n", inputfileGroup[i]);
                        exit(1);
                }

                // get the file size
                fseek(rFile[i], 0, SEEK_END);
                size[i] = ftell(rFile[i]);
                rewind(rFile[i]);
                printf("The file size of %s is %ld\n", inputfileGroup[i], size[i]);
        }


	for(i=0; i<inputNum; i++) {
		// allocate memory for file reading
		ibuff[i] = malloc(size[i]);
		if(ibuff[i] == NULL) {
			printf("memory allocation error for file %s\n", inputfileGroup[i]);
			exit(1);
		}

		// read the file into memory
		long rv = fread(ibuff[i],sizeof(char),size[i],rFile[i]);
		if(rv!=size[i]) {
			printf("file reading error for file %s\n", inputfileGroup[i]);
			exit(1);
		}

		// allocate buffer for the output: obuff;
		outlen[i] = snappy_max_compressed_length(size[i]);
		obuff[i] = malloc(outlen[i]);
		if(obuff[i] == NULL) {
			printf("memory allocation error for the output file of %s\n", inputfileGroup[i]);
			exit(1);
		}

		// compress from ibuff[i] to obuff[i]
		struct snappy_env env;
		snappy_init_env(&env);
		int err = snappy_compress(&env, ibuff[i], size[i], obuff[i], &outlen[i]);
		snappy_free_env(&env);

		// check whether error occurs
		if (err) {
			printf("compression error: %d\n",err);
			exit(1);
		}
	}


	// write the output file
	FILE * wFile = fopen(outputfile,"wb");
	// file format: (Number of files)(size of each file ...)(file1,file2,...)
	// 1. meta data (The number of files in one combined compressed file)
	long rv = fwrite(&inputNum, sizeof(inputNum), 1, wFile);
	if(rv!=1) {
		printf("file writing error for the number of files\n");
		exit(1);
	}

	// 2. meta data (The size of each compressed file)
	rv = fwrite(outlen, sizeof(size_t), inputNum, wFile);
	if(rv!=inputNum) {
		printf("file writing error for each file size\n");
		exit(1);
	}
	for(i=0; i<inputNum; i++) {
		long rv = fwrite(obuff[i],sizeof(char),outlen[i],wFile);
		if(rv!=outlen[i]) {
			printf("file writing error for compressed file of %s\n", inputfileGroup[i]);
			exit(1);
		}
	}

	// close the files
	for(i=0; i<inputNum; i++) {
		fclose(rFile[i]);
	}
	fclose(wFile);

	// free the memory allocation
	for(i=0; i<inputNum; i++) {
		free(ibuff[i]);
		free(obuff[i]);	
	}

	return 0;
}
