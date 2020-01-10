#define _XOPEN_SOURCE 600
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <pthread.h>
#include "get_num.h"


// 屏障
static pthread_barrier_t barrier;
// 屏障的个数
static int num_barriers;

/**
 * 
 */
static void *
thr_func(void *arg)
{
    int s;
    int nsecs;
    long thread_num = (long)arg;

    printf("thread %ld started\n", thread_num);

    srandom(time(NULL) + thread_num);

    // 对每个线程，都要通过多个屏障，对每个屏障，多个线程是同时通过
    for (int i=0; i<num_barriers; i++)
    {
        nsecs = random() % 5 + 1;
        sleep(nsecs);

        printf("thread %ld about to wait on barrier %d after sleeping %d seconds\n", thread_num, i, nsec);

        // 每个线程都会在此等待，然后一起同时结束
        s = pthread_barrier_wait(&barrier);
        if (0 == s)
        {
            printf("thread %ld passed barrier %d, return value was 0\n", thread_num, i);
        }
        else if (PTHREAD_BARRIER_SERIAL_THREAD == s)
        {
            printf("thread %ld passed barrier %d, return value was PTHREAD_BARRIER_SERIAL_THREAD\n", thread_num, i);

            usleep(100000);
            printf("\n");
        }
        else
        {
            perror("pthread_barrier_wait error");
        }        
    }

    usleep(200000);
    printf("thread %ld terminated \n", thread_num);
    return NULL;
}


/**
 *    
 * 
 * cc -g -Wall -o thread_barrier thread_barrier.c -lpthread
 num_barriers = 1, num_threads = 3
thread 0 started
thread 2 started
thread 1 started

thread 0 about to wait on barrier 0 after sleeping 1 seconds
thread 1 about to wait on barrier 0 after sleeping 4 seconds
thread 2 about to wait on barrier 0 after sleeping 5 seconds
thread 2 passed barrier 0, return value was PTHREAD_BARRIER_SERIAL_THREAD
thread 0 passed barrier 0, return value was 0
thread 1 passed barrier 0, return value was 0

thread 0 terminated 
thread 1 terminated 
thread 2 terminated 



./thread_barrier 2 3
num_barriers = 2, num_threads = 3
thread 0 started
thread 1 started
thread 2 started

thread 0 about to wait on barrier 0 after sleeping 1 seconds
thread 1 about to wait on barrier 0 after sleeping 2 seconds
thread 2 about to wait on barrier 0 after sleeping 4 seconds
thread 2 passed barrier 0, return value was PTHREAD_BARRIER_SERIAL_THREAD
thread 1 passed barrier 0, return value was 0
thread 0 passed barrier 0, return value was 0

thread 1 about to wait on barrier 1 after sleeping 2 seconds
thread 0 about to wait on barrier 1 after sleeping 2 seconds
thread 2 about to wait on barrier 1 after sleeping 3 seconds
thread 2 passed barrier 1, return value was PTHREAD_BARRIER_SERIAL_THREAD
thread 0 passed barrier 1, return value was 0
thread 1 passed barrier 1, return value was 0

thread 0 terminated 
thread 1 terminated 
thread 2 terminated 

 */
int
main(int argc, char *argv[])    
{
    int s;
    int num_threads;
    pthread_t *tid;

    if (argc != 3 || 0 == strcmp(argv[1], "--help"))
        fprintf(stderr, "%s num-barriers num-threads \n", argv[0]);

    num_barriers = atoi(argv[1]);
    num_threads = atoi(argv[2]);

    printf("num_barriers = %d, num_threads = %d\n", num_barriers, num_threads);
    
    // 线程分配内存
    if (NULL == (tid = calloc(sizeof(pthread_t), num_threads)))
        perror("calloc error");

    // 初始化屏障
    if (0 != (s = pthread_barrier_init(&barrier, NULL, num_threads)))
        perror("pthread_barrier_init error");

    // 创建多个线程
    for (long idx=0; idx<num_threads; idx++)
    {
        if (0 != (s = pthread_create(&tid[idx], NULL, thr_func, (void *)idx)))
            perror("pthread_create error");
    }

    usleep(10000);
    printf("\n");

    // 主线程等待多个线程结束
    for (int i=0; i<num_threads; i++)
    {
        if (0 != (s = pthread_join(tid[i], NULL)))
            perror("pthread_join error");
    }

    exit(EXIT_SUCCESS);
}
