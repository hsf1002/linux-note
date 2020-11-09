#define _POSIX_C_SOURCE 199309
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <sys/time.h>
#include "cur_time.h"


static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
// 过期定时器的数量
static int exprire_cnt = 0;


/**
 * 
 * 将时间+间隔，转为itimerspec的值
 */
static void
itimerspec_from_str(const char *str, struct itimerspec *tsp)
{
    char *dupstr;
    char *cptr;
    char *sptr;

    dupstr = strdup(str);

    if (NULL != (cptr = strchr(dupstr, ':')))
        *cptr = '\0';
    
    if (NULL != (sptr = strchr(dupstr, '/')))
        *sptr = '\0';
    
    tsp->it_value.tv_sec = atoi(dupstr);
    tsp->it_value.tv_nsec = (sptr != NULL) ? atoi(sptr + 1) : 0;

    if (NULL == cptr)
    {
        tsp->it_interval.tv_sec = 0;
        tsp->it_interval.tv_nsec = 0;
    }
    else
    {
        if (NULL != (sptr = strchr(cptr, '/')))
            *sptr = '\0';

        tsp->it_interval.tv_sec = atoi(cptr + 1);
        tsp->it_interval.tv_nsec = (sptr != NULL) ? atoi(sptr + 1) : 0;
    }
}

/**
 * 4. 定时器到期调用此函数
 */
static void
thread_fun(union sigval sv)
{
    timer_t *tidptr;
    int s;

    tidptr = sv.sival_ptr;

    printf("[%s] Thread notify \n", curr_time("%T"));
    printf("    timer ID = %ld\n", (long)*tidptr);
    printf("    timer_getoverrun() = %d \n", timer_getoverrun(*tidptr));

    if (0 != (s = pthread_mutex_lock(&mtx)))
        perror("mutex lock error");
    
    exprire_cnt += 1 + timer_getoverrun(*tidptr);

    if (0 != (s = pthread_mutex_unlock(&mtx)))
        perror("mutex unlock error");
    
    // 条件满足，通知主线程
    if (0 != (s = pthread_cond_signal(&cond)))
        perror("cond signal error");
}

/**
 *   
 * 
 * 使用线程函数进行POSIX定时器通知
 * 
 * 编译要添加实时库：cc -g -Wall -o sigev_thread sigev_thread.c libcurtime.so -lrt
 * 
LD_LIBRARY_PATH=. ./ptmr_sigev_thread 1:3 
Timer ID: 37208432 (1:3) 
[10:00:58] Thread notify 
    timer ID = 37208432
    timer_getoverrun() = 0 
main: expire cnt = 1 
[10:01:01] Thread notify 
    timer ID = 37208432
    timer_getoverrun() = 0 
main: expire cnt = 2 
[10:01:04] Thread notify 
    timer ID = 37208432
    timer_getoverrun() = 0 
main: expire cnt = 3 
[10:01:07] Thread notify 
    timer ID = 37208432
    timer_getoverrun() = 0 
main: expire cnt = 4 
[10:01:10] Thread notify 
    timer ID = 37208432
    timer_getoverrun() = 0 
main: expire cnt = 5 
^Z
[2]+  已停止               LD_LIBRARY_PATH=. ./ptmr_sigev_thread 1:3

fg
LD_LIBRARY_PATH=. ./ptmr_sigev_thread 1:3
[10:01:31] Thread notify 
    timer ID = 37208432
    timer_getoverrun() = 5 
main: expire cnt = 11 
[10:01:31] Thread notify 
    timer ID = 37208432
    timer_getoverrun() = 0 
main: expire cnt = 12 
[10:01:34] Thread notify 
    timer ID = 37208432
    timer_getoverrun() = 0 
main: expire cnt = 13 
^C

 */
int
main(int argc, char *argv[])    
{
    struct itimerspec ts;
    struct sigevent sev;
    timer_t *tidlist;
    int s;

    if (argc < 2)
        fprintf(stderr, "%s sec[/nsec][:int-sec][/int-nsec]...\n", argv[0]);
    
    // 根据指定参数，给多个定时器分配空间
    if (NULL == (tidlist = calloc(argc - 1, sizeof(timer_t))))
        perror("calloc error");

    // 通过线程函数发送通知
    sev.sigev_notify = SIGEV_THREAD;
    // 指定线程函数
    sev.sigev_notify_function = thread_fun;
    sev.sigev_notify_attributes = NULL;

    // 1. 启动若干个定时器
    for (int i=0; i<argc-1; i++)
    {
        // 命令行指定参数转换为itimerspec结构
        itimerspec_from_str(argv[i+1], &ts);

        // 指定伴随数据为定时器的timer_id
        sev.sigev_value.sival_ptr = &tidlist[i];

        // 创建定时器
        if (-1 == timer_create(CLOCK_REALTIME, &sev, &tidlist[i]))
            perror("timer_create error");
        
        printf("Timer ID: %ld (%s) \n", (long)tidlist[i], argv[i+1]);

        // 启动定时器
        if (-1 == timer_settime(tidlist[i], 0, &ts, NULL))
            perror("timer_settime error");
    }

    // 2. 互斥量加锁
    if (0 != (s = pthread_mutex_lock(&mtx)))
        perror("mutex lock error");
    
    // 3. 阻塞
    for (;;)
    {
        // 等待满足条件变量
        if (0 != (s = pthread_cond_wait(&cond, &mtx)))
            perror("cond wait error");
        
        printf("main: expire cnt = %d \n", exprire_cnt);
    }

    exit(EXIT_SUCCESS);
}
