#define _GNU_SOURCE
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


static volatile sig_atomic_t got_alarm = 0; // for SIGALRM

/**
 * 
 * 
 */
static void
display_times(const char *msg, bool include_timer)
{
    struct itimerval itv;
    static struct timeval start;
    struct timeval curr;
    static int call_num = 0;

    // 第一次调用此函数的起始时间
    if (0 == call_num)
        if (gettimeofday(&start, NULL))
            perror("gettimeofday error");
    
    if (call_num % 20 == 0)
        printf("        Elapsed  value  interval \n");
    
    // 第一次以后每次调用此函数的时间
    if (-1 == gettimeofday(&curr, NULL))
        perror("gettimeofday error");

    // 
    printf("%-7s %6.2f", msg, curr.tv_sec - start.tv_sec + (curr.tv_usec - start.tv_usec)/1000000.0);

    if (include_timer)
    {
        // 获取当前定时器信息
        if (-1 == getitimer(ITIMER_REAL, &itv))
            perror("getitimer error");
        
        printf("  %6.2f %6.2f", itv.it_value.tv_sec + itv.it_value.tv_usec / 1000000.0, \
            itv.it_interval.tv_sec + itv.it_interval.tv_usec / 1000000.0);
    }

    printf("\n");
    call_num++;
}

/**
 * 
 */
static void
sigalrm_handler(int signo)
{
    got_alarm = 1;
}


/**
 *   
 * 
 * 
 * 
skydeiMac:23-定时器与休眠 sky$ ./realtimer 1 800000 1 0
        Elapsed  value  interval 
START:    0.00
Main:     0.50    1.30   1.00
Main:     1.00    0.80   1.00
Main:     1.50    0.30   1.00
ALARM:    1.80    1.00   1.00
Main:     2.00    0.80   1.00
Main:     2.51    0.29   1.00
ALARM:    2.80    1.00   1.00
that's all folks
skydeiMac:23-定时器与休眠 sky$ ./realtimer 3 800000 2 0
        Elapsed  value  interval 
START:    0.00
Main:     0.50    3.30   2.00
Main:     1.00    2.80   2.00
Main:     1.50    2.30   2.00
Main:     2.00    1.80   2.00
Main:     2.51    1.29   2.00
Main:     3.01    0.79   2.00
Main:     3.51    0.29   2.00
ALARM:    3.80    2.00   2.00
Main:     4.01    1.79   2.00
Main:     4.51    1.29   2.00
Main:     5.01    0.79   2.00
Main:     5.51    0.29   2.00
ALARM:    5.80    2.00   2.00
that's all folks




 */
int
main(int argc, char *argv[])    
{
    struct itimerval itv;
    clock_t prev_clock;
    int max_sigs;
    int sig_cnt;
    struct sigaction sa;

    if (argc > 1 && 0 == strcmp(argv[1], "--help"))
        fprintf(stderr, "%s sec [usecs [int-secs [int-usecs]]]]\n", argv[0]);

    sig_cnt = 0;

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = sigalrm_handler;

    if (-1 == sigaction(SIGALRM, &sa, NULL))
        perror("sigaction error");

    // 一次性定时器，只响一次，周期性闹钟，响三次
    max_sigs = (itv.it_interval.tv_sec == 0 && itv.it_interval.tv_usec == 0) ? 1 : 3;

    display_times("START:", false);

    itv.it_value.tv_sec = (argc > 1) ? getLong(argv[1], 0, "sec") : 2;
    itv.it_value.tv_usec = (argc > 2) ? getLong(argv[2], 0, "usec") : 0;
    itv.it_interval.tv_sec = (argc > 3) ? getLong(argv[3], 0, "int-sec") : 0;
    itv.it_interval.tv_usec = (argc > 4) ? getLong(argv[4], 0, "int-usec") : 0;

    if (-1 == setitimer(ITIMER_REAL, &itv, 0))
        perror("setitimer error");
    
    // 当前CPU时钟
    prev_clock = clock();
    sig_cnt = 0;

    for (;;)
    {
        // 0.5秒的时钟频率，打印一次信息
        while(((clock() - prev_clock) * 10 / CLOCKS_PER_SEC) < 5)
        {
            // 使用全局变量控制，可以避免在信号处理函数中调用非异步信号安全的函数
            if (got_alarm)
            {
                got_alarm = 0;

                display_times("ALARM:", true);

                sig_cnt++;

                if (sig_cnt > max_sigs)
                {
                    printf("that's all folks\n");
                    exit(EXIT_SUCCESS);
                }
            }
        }

        prev_clock = clock();
        display_times("Main:", true);
    }

    exit(EXIT_SUCCESS);
}

