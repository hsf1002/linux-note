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
#include <stdint.h>
#include "cur_time.h"
#include "get_num.h"

/**
 * 
 * 将时间+间隔，转为itimerspec的值
 */
static void
itimerspec_from_str(const char *str, struct itimerspec *tsp)
{
    char *cptr;
    char *sptr;
    char *dupstr;

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
        if (NULL != (sptr = strchr(cptr + 1, '/')))
            *sptr = '\0';

        tsp->it_interval.tv_sec = atoi(cptr + 1);
        tsp->it_interval.tv_nsec = (sptr != NULL) ? atoi(sptr + 1) : 0;
    }
}

/**
 *   
 * 
 * 使用文件描述符进行POSIX定时器通知

cc -g -Wall -o timer_fd demo_timerfd.c libgetnum.so libcurtime.so

hefeng@sw-hefeng:/home/workspace1/logs/test$ LD_LIBRARY_PATH=. ./timer_fd 5:10 4
14:11:45 start
14:11:50  5.  0: expiration read: 1; total=1
14:12:00  15.  0: expiration read: 1; total=2
14:12:10  25.  0: expiration read: 1; total=3
14:12:20  35.  0: expiration read: 1; total=4
hefeng@sw-hefeng:/home/workspace1/logs/test$ LD_LIBRARY_PATH=. ./timer_fd 10:5 4
14:12:36 start
14:12:46  10.  0: expiration read: 1; total=1
14:12:51  15.  0: expiration read: 1; total=2
14:12:56  20.  0: expiration read: 1; total=3
14:13:01  24.1000: expiration read: 1; total=4
hefeng@sw-hefeng:/home/workspace1/logs/test$ LD_LIBRARY_PATH=. ./timer_fd 10:5 4
14:14:17 start
14:14:27  10.  0: expiration read: 1; total=1
^Z
[1]+  已停止               LD_LIBRARY_PATH=. ./timer_fd 10:5 4
hefeng@sw-hefeng:/home/workspace1/logs/test$ fg
LD_LIBRARY_PATH=. ./timer_fd 10:5 4
14:14:38  20.942: expiration read: 2; total=3
14:14:42  25.  0: expiration read: 1; total=4

 */
int
main(int argc, char *argv[])    
{
    struct itimerspec ts;
    struct timespec start, now;
    int max_exp, fd, secs, nanosecs;
    uint64_t num_exp, total_exp;
    ssize_t s;

    if (argc < 2 || 0 == strcmp(argv[1], "--help"))
        fprintf(stderr, "%s sec[/nsec][:int-sec][/int-nsec][max-exp]...\n", argv[0]);
    
    // 命令行指定参数转换为itimerspec结构
    itimerspec_from_str(argv[1], &ts);

    // 指定定时器过期的总次数
    max_exp = (argc > 2) ? getInt(argv[2], GN_GT_0, "max-exp") : 1;

    // 创建定时器
    if (-1 == (fd = timerfd_create(CLOCK_REALTIME, 0)))
        perror("timerfd_create error");
    
    // 设置定时器
    if (-1 == timerfd_settime(fd, 0, &ts, NULL))
        perror("timerfd_settimer error");
    
    // 获取起始时间
    if (-1 == clock_gettime(CLOCK_MONOTONIC, &start))
        perror("clock_gettime error");

    printf("%s start\n", curr_time("%T"));

    // 把每次过期的信息都打印出来
    for (total_exp=0; total_exp<max_exp;)
    {   
        // 周期性定时器，只要过期一次，就会返回
        if (sizeof(uint64_t) != (s = read(fd, &num_exp, sizeof(uint64_t))))
            perror("read error");
        
        total_exp += num_exp;

        if (-1 == clock_gettime(CLOCK_MONOTONIC, &now))
            perror("clock_gettime error");
        
        secs = now.tv_sec - start.tv_sec;
        nanosecs = now.tv_nsec - start.tv_nsec;

        if (nanosecs < 0)
        {
            secs--;
            nanosecs += 1000000000;
        }
        
        printf("%s  %d.%3d: expiration read: %llu; total=%llu\n", curr_time("%T"), secs, (nanosecs + 500000) / 1000000, \
            (unsigned long long)num_exp, (unsigned long long)total_exp);
    }

    exit(EXIT_SUCCESS);
}
