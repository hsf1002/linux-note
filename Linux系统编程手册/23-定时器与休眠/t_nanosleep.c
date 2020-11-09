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
#include "get_num.h"


/**
 * 
 */
static void
handler(int signo)
{
    printf("caught signal \n");
    return;
}


/**
 *   
 * 
 * 使用nanosleep
 * 

skydeiMac:23-定时器与休眠 sky$ ./t_nanosleep 10 5000
^Ccaught signal 
nanosleep interrupted: Interrupted system call
slept for:  1.745407 sec 
remaining:  8.254598762
^Ccaught signal 
nanosleep interrupted: Interrupted system call
slept for:  2.880830 sec 
remaining:  7.119213658
^Ccaught signal 
nanosleep interrupted: Interrupted system call
slept for:  3.817011 sec 
remaining:  6.183046227
^Ccaught signal 
nanosleep interrupted: Interrupted system call
slept for:  4.904942 sec 
remaining:  5.095141425
^Ccaught signal 
nanosleep interrupted: Interrupted system call
slept for:  9.624827 sec 
remaining:  0.375207557
slept for: 10.005106 sec

 */
int
main(int argc, char *argv[])    
{
    struct timeval start;
    struct timeval finish;
    struct timespec request;
    struct timespec remain;
    struct sigaction sa;
    int s;

    if (argc != 3 || 0 == strcmp(argv[1], "--help"))
        fprintf(stderr, "%s sec nanosec\n", argv[1]);
    
    request.tv_sec = getLong(argv[1], 0, "sec");
    request.tv_nsec = getLong(argv[2], 0, "nanosleep");

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = handler;

    // 为SIGINT注册信号处理函数
    if (-1 == sigaction(SIGINT, &sa, NULL))
        perror("sigaction error");
    
    if (-1 == gettimeofday(&start, NULL))
        perror("gettimeofday error");
    
    for (;;)
    {
        s = nanosleep(&request, &remain);

        // 被信号中断
        if (-1 == s && errno == EINTR)
            perror("nanosleep interrupted");
        
        if (-1 == gettimeofday(&finish, NULL))
            perror("getimeofday error");

        // 已经休眠的时间
        printf("slept for: %9.6f sec \n", finish.tv_sec - start.tv_sec + (finish.tv_usec - start.tv_usec) / 1000000.0);


        if (0 == s)
            break;
        
        // 剩余休眠的时间
        printf("remaining: %2ld.%09ld\n", (long)remain.tv_sec, (long)remain.tv_nsec);

        // 如果被中断，继续休眠，直到达到指定时间
        request = remain;
    }

    printf("sleep complete!");

    exit(EXIT_SUCCESS);
}

