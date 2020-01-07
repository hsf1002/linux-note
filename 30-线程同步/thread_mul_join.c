
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


static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

// 线程总数
static int total_thr = 0;
// 运行的线程总数
static int num_lived = 0;
// 已终止尚未连接的线程总数
static int num_unjoined = 0;

enum thr_state
{
    // 运行中
    TS_ALIVE,
    // 已终止，尚未连接
    TS_TERMINATED,
    // 已终止，已经连接
    TS_JOINED,
};

static struct
{
    pthread_t tid;
    enum thr_state state;
    int sleep_time;
}*thread;

/**
 * 
 */
static void *
thr_func(void *arg)
{
    int idx = (int)arg;
    int s;

    // 每个线程睡眠一段指定时间
    sleep(thread[idx].sleep_time);

    printf("thread %d terminating \n", idx);

    if (0 != (s = pthread_mutex_lock(&mtx)))
        perror("pthread_mutex_lock error");
    
    // 线程状态改变为TS_TERMINATED
    num_unjoined++;
    thread[idx].state = TS_TERMINATED;

    if (0 != (s = pthread_mutex_unlock(&mtx)))
        perror("pthread_mutex_unlock error");
    
    if (0 != (s = pthread_cond_signal(&cond)))
        perror("pthread_cond_signal error");
    
    return NULL;
}


/**
 *    pthread_join只能连接一个指定线程，且无任何机制去连接已终止的线程，本例通过条件变量绕开这个限制
 * 

./thread_mul_join 1 2 2 3 4
total_thread = 5 
thread 0 terminating 
reaped thread 0 (num_lived = 4) 
thread 1 terminating 
thread 2 terminating 
reaped thread 1 (num_lived = 3) 
reaped thread 2 (num_lived = 2) 
thread 3 terminating 
reaped thread 3 (num_lived = 1) 
thread 4 terminating 
reaped thread 4 (num_lived = 0) 

*/
int
main(int argc, char *argv[])    
{
    int idx;
    int s;

    if (argc < 2 || 0 == strcmp(argv[1], "--help"))
        fprintf(stderr, "%s nsec...\n", argv[0]);
    
    if (NULL == (thread = calloc(argc - 1, sizeof(*thread))))
        perror("calloc error");

    // 开始创建多个线程，默认状态为TS_ALIVE
    for (idx=0; idx<argc-1; idx++)
    {
        thread[idx].sleep_time = getInt(argv[idx+1], GN_NONNEG, NULL);
        thread[idx].state = TS_ALIVE;
        if (0 != (s = pthread_create(&thread[idx].tid, NULL, thr_func, (void *)idx)))
            perror("pthread_create error");
    }

    total_thr = argc - 1;
    num_lived = total_thr;

    printf("total_thread = %d \n", total_thr);

    // 连接已经终止的线程
    while (num_lived > 0)
    {
        if (0 != (s = pthread_mutex_lock(&mtx)))
            perror("pthread_mutex_lock error");
        
        // 如果没有已终止，尚未连接的线程，就睡眠等待
        while (num_unjoined == 0)
        {
            if (0 != (s = pthread_cond_wait(&cond, &mtx)))
                perror("pthread_cond_wait error");
        }

        // 遍历寻找已终止的线程，连接，并修改状态为TS_JOINED
        for (idx=0; idx<total_thr; idx++)
        {
            if (thread[idx].state == TS_TERMINATED)
            {
                if (0 != (s = pthread_join(thread[idx].tid, NULL)))
                    perror("pthread_join error");
                
                thread[idx].state = TS_JOINED;
                num_lived--;
                num_unjoined--;

                printf("reaped thread %d (num_lived = %d) \n", idx, num_lived);
            }
        }
        
        if (0 != (s = pthread_mutex_unlock(&mtx)))
            perror("pthread_mutex_unlock error");
    }

    exit(EXIT_SUCCESS);
}

