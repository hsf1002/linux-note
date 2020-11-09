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
#include <sys/time.h>
#include "cur_time.h"


#define TIMER_SIG SIGRTMAX


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
 * 
 */
static void
handler(int signo, siginfo_t *si, void *uc)
{
    timer_t *tidptr;

    tidptr = si->si_value.sival_ptr;

    printf("[%s] got signal %d \n", curr_time("%T"), signo);
    printf("    *sival_ptr         = %ld\n", (long)*tidptr);
    printf("    timer_getoverrun() = %ld\n", timer_getoverrun(*tidptr));
}


/**
 *   
 * 
 * 使用信号进行POSIX定时器通知
 * 编译要添加实时库：cc -g -Wall -o ptmr_sigev_signal ptmr_sigev_signal.c libcurtime.so -lrt

LD_LIBRARY_PATH=. ./ptmr_sigev_signal 1:3
Timer ID: 21168208 (1:3) 
[09:49:38] got signal 64 
    *sival_ptr         = 21168208
    timer_getoverrun() = 0
[09:49:41] got signal 64 
    *sival_ptr         = 21168208
    timer_getoverrun() = 0
[09:49:44] got signal 64 
    *sival_ptr         = 21168208
    timer_getoverrun() = 0
[09:49:47] got signal 64 
    *sival_ptr         = 21168208
    timer_getoverrun() = 0
^Z
[2]+  已停止               LD_LIBRARY_PATH=. ./ptmr_sigev_signal 1:3

fg
LD_LIBRARY_PATH=. ./ptmr_sigev_signal 1:3
[09:50:06] got signal 64 
    *sival_ptr         = 21168208
    timer_getoverrun() = 5
[09:50:08] got signal 64 
    *sival_ptr         = 21168208
    timer_getoverrun() = 0
[09:50:11] got signal 64 
    *sival_ptr         = 21168208
    timer_getoverrun() = 0
[09:50:14] got signal 64 
    *sival_ptr         = 21168208
    timer_getoverrun() = 0
^C
 */
int
main(int argc, char *argv[])    
{
    struct itimerspec ts;
    struct sigaction sa;
    struct sigevent sev;
    timer_t *tidlist;

    if (argc < 2)
        fprintf(stderr, "%s sec[/nsec][:int-sec][/int-nsec]...\n", argv[0]);
    
    // 根据指定参数，给多个定时器分配空间
    if (NULL == (tidlist = calloc(argc - 1, sizeof(timer_t))))
        perror("calloc error");

    // 带此标志信号处理函数才能带3个参数
    sa.sa_flags = SA_SIGINFO;
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    // 为实时信号注册信号处理函数
    if (-1 == sigaction(TIMER_SIG, &sa, NULL))
        perror("sigaction error");

    // 通过信号发送通知
    sev.sigev_notify = SIGEV_SIGNAL;
    // 信号编号
    sev.sigev_signo = TIMER_SIG;

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
    
    // 阻塞，等待信号
    for (;;)
        pause();

    exit(EXIT_SUCCESS);
}
