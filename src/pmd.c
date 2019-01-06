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
#define __USE_GNU		// For <sched.h>
#include <sched.h>		// Might need to place this before <pthread.h>
#include <pthread.h>		// since pthread.h also include sched.h

#ifndef CACHELINE
#define CACHELINE 64
#endif

#ifndef MAX_NUM_OUTPUT
#define MAX_NUM_OUTPUT 128
#endif

#ifndef MAX_THREAD_NUM
#define MAX_THREAD_NUM 128
#endif

#ifndef TIMINGINFO
#define TIMINGINFO 1
#endif

//-------------------------------  Type Definitions  -------------------------// 
typedef struct args_t args_t;
typedef struct dec_arg_t dec_arg_t;
typedef struct task_t task_t;
typedef struct task_list_t task_list_t;
typedef struct queue_t queue_t;

//------------------------------- Task Related Structures --------------------// 
struct args_t {
	queue_t* file_queue;
	int nthreads;
	int my_tid;
};

struct dec_arg_t {
	char* src;		// the input buffer address
	char* des;		// the output buffer address
	size_t input_size;	// the size of the input file (buffer): size of one snappy compressed file
	size_t output_size;	// the size of the output buffer
};

struct task_t {
	dec_arg_t dec_arg;	
	
	// next pointer
	task_t *next;
};

struct task_list_t {
	task_t *	tasks;	// the head of the task_list_t which is a task_t, tasks is in array format
	task_list_t *	next;	// the next task_list_t
	int		count;	// count how many task space is used
};

struct queue_t {
	pthread_mutex_t	lock;		// lock for multi-thread operation
	task_t *	head;		// the head of the task
	task_list_t *	free_list;	// the free_list for the task
	int		count;		// how many tasks are in the queue
	int		set_list_size;	// a fixed(set) size of the free_list
};

//------------------------------- For Mapping The CPUs  --------------------// 
//static const int mapping[40]={0,1,2,3,4,5,6,7,8,9,20,21,22,23,24,25,26,27,28,29,10,11,12,13,14,15,16,17,18,19,30,31,32,33,34,35,36,37,38,39};
static const int mapping[40]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39};

int get_cpuid(int i)
{
	return mapping[i];
}

//------------------------------- Task Related Functions --------------------// 
queue_t * queue_initial(int set_list_size) {
	queue_t * queue		=	(queue_t*) malloc(sizeof(queue_t));
	queue->free_list	=	(task_list_t*) malloc(sizeof(task_list_t));
	queue->free_list->tasks	=	(task_t*) malloc(set_list_size * sizeof(task_t));
	queue->free_list->next	=	NULL;
	queue->free_list->count	=	0;

	queue->head		=	NULL;
	queue->count		=	0;
	queue->set_list_size	=	set_list_size;
	pthread_mutex_init(&queue->lock, NULL);

	return queue;
}

// Free the queue. Make sure to free the task_list_t before the free of the queue itself
// Also, make sure the tasks array is free before the task_list_t is free
void queue_free(queue_t * queue) {
	// Free the task_list_t first before free the queue_t
	task_list_t * current	=	queue->free_list;
	while(current) {
		// Similarly, free the tasks in the task_list_t before free the task_list_t
		free(current->tasks);
		task_list_t * next	=	current->next;
		free(current);
		current			=	next;
	}

	// Now free the queue itself
	free(queue);
}

// Get a slot of task from the free_list in the queue
// If the free list still has space, just used it, and record the count
// If not, allocate new free_list, and use the first space of the tasks
inline __attribute__((always_inline)) task_t * queue_get_slot(queue_t * queue) {
	task_t * task;
	if(queue->free_list->count < queue->set_list_size) {
		task	=	&(queue->free_list->tasks[queue->free_list->count]);
		queue->free_list->count ++;
	} else {
		task_list_t * new_list	=	(task_list_t*) malloc(sizeof(task_list_t));
		new_list->tasks		=	(task_t*) malloc(queue->set_list_size * sizeof(task_t));
		new_list->next		=	queue->free_list;
		queue->free_list	=	new_list;
		new_list->count		=	1;
		task			=	&(new_list->tasks[0]);
	}

	return task;
}

// Add a task to the queue: add the task to the head of the queue and change the count
inline __attribute__((always_inline)) void queue_add_task(queue_t * queue, task_t * task) {
	task->next	=	queue->head;
	queue->head	=	task;
	queue->count ++;
}

// Get a task from the queue: a reverse operation of queue_add_task
// This function need to be atomic
inline __attribute__((always_inline)) task_t * queue_get_task(queue_t * queue) {
	// obtain the locker before operating on it
	pthread_mutex_lock(&queue->lock);
	task_t * task = 0;
	if(queue->count > 0) {
		task		=	queue->head;
		queue->head	=	task->next;
		queue->count --;
	}
	pthread_mutex_unlock(&queue->lock);
	
	return task;
}

// Thread function
void * snappy_thread(void* thread_arg) {
	args_t * args		=	(args_t*) thread_arg;
	int my_tid		=	args->my_tid;
	queue_t * file_queue	=	args->file_queue;

	task_t * file_task;
	while(file_task	= queue_get_task(file_queue)) {
		char* input		=	file_task->dec_arg.src;
		char* output		=	file_task->dec_arg.des;
		size_t size		=	file_task->dec_arg.input_size;

		int err = snappy_uncompress(input,size,output);
		// check whether error occurs
		if (err) {
			printf("decompression error: %d\n",err);
			exit(1);
		}
	}
}

//------------------------------- The Start Of The Main --------------------// 
int main(int ac, char **av)
{
	if(ac < 4 || ac > 4) {
		printf("Parameter error! Please use:\n");
		printf("./exe numOfThread input output\n");
		return -1;
	}

	// record the number of input files, 1 for exe, 1 for output file	
	const int nthreads = atoi(av[1]);
	const char * inputfile = av[2];
	const char * outputfile = av[3];

	// Set the process (thread 0) start from CPU 0
	cpu_set_t startset;
	CPU_ZERO(&startset);
	CPU_SET(0, &startset);
	if(sched_setaffinity(0, sizeof(startset), &startset) <0) {
		perror("sched_setaffinity\n");
	}

        int i=0;
	int outputNum;
	size_t totalFileSize = 0;
        size_t size[MAX_NUM_OUTPUT];
        size_t outlen[MAX_NUM_OUTPUT];
	char *ibuff[MAX_NUM_OUTPUT], *obuff[MAX_NUM_OUTPUT];

	pthread_t tid[MAX_THREAD_NUM];
	pthread_attr_t attr;
	cpu_set_t set;
	args_t args[MAX_THREAD_NUM];

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

	// Use multi-thread from here
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

	// add the input buffers to the queue
	queue_t * file_queue = queue_initial(MAX_NUM_OUTPUT);
	for(i=0; i<outputNum; i++) {
		// get the task slot
		task_t * file_task	=	queue_get_slot(file_queue);
		
		// set the task parameters
		file_task->dec_arg.src		=	ibuff[i];
		file_task->dec_arg.des		=	obuff[i];
		file_task->dec_arg.input_size	=	size[i];
		file_task->dec_arg.output_size	=	outlen[i];

		// add the task to the queue
		queue_add_task(file_queue, file_task);
	}

#ifdef TIMINGINFO
	gettimeofday(&timer1,NULL);
#endif
	// get cpu number of a machine
	int num_cpus = sysconf( _SC_NPROCESSORS_ONLN );
	// create threads for decompression
	for(i=0; i<nthreads; i++) {
		int cpu_idx = get_cpuid(i%num_cpus);
		CPU_ZERO(&set);
		CPU_SET(cpu_idx,&set);
		pthread_attr_setaffinity_np(&attr,sizeof(cpu_set_t),&set);

		args[i].my_tid		=	i;
		args[i].nthreads	=	nthreads;
		args[i].file_queue	=	file_queue;

		rv = pthread_create(&tid[i], &attr, snappy_thread, (void*)&args[i]);
		if (rv){
			printf("[ERROR] return code from pthread_create() is %ld\n", rv);
			exit(-1);
		}
	}

	// Join all the threads
	for(i = 0; i < nthreads; i++) {
		pthread_join(tid[i], NULL);
	}
#ifdef TIMINGINFO
	gettimeofday(&timer2,NULL);
	long runningtimeAll = (timer2.tv_sec-timer0.tv_sec)*1000000L+timer2.tv_usec-timer0.tv_usec;
	long runningtime = (timer2.tv_sec-timer1.tv_sec)*1000000L+timer2.tv_usec-timer1.tv_usec;
	printf("Decompression time is %ld usec with allocation cost and %ld usec without\n", runningtimeAll, runningtime);
	double throughputAll = (double)totalFileSize/runningtimeAll/1.024/1.024;
	double throughput = (double)totalFileSize/runningtime/1.024/1.024;
	printf("The throughput (with memory allocation) is:\t %f MB/s\n", throughputAll);
	printf("The throughput (without memory allocation) is:\t %f MB/s\n", throughput);
#endif

#ifndef NOFILEWRITE
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
#endif

	// free the ibuff and obuff
	for(i=0; i<outputNum; i++) {
		free(ibuff[i]);
		free(obuff[i]);	
	}
	queue_free(file_queue);
	
	// remember to close the file
	fclose(rFile);

	return 0;
}
