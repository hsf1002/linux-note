#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <time.h>
#include "get_num.h"


/**
 * 
 *  按照格式化要求显示时间格式
 */
char *
curr_time(const char *format)
{
    static char buf[BUFSIZ];  
    time_t t;
    size_t s;
    struct tm *tm;

    t = time(NULL);
    tm = localtime(&t);

    if (tm == NULL)
        return NULL;

    s = strftime(buf, BUFSIZ, (format != NULL) ? format : "%c", tm);

    return (s == 0) ? NULL : buf;
}

/**
 *   创建并等待多个子进程
 * 
cc -g -Wall -o multi_wait multi_wait.c libgetnum.so

./multi_wait 7 1 4
10:22:46 child 1 started with PID = 69189, sleeping 7 second
10:22:46 child 2 started with PID = 69190, sleeping 1 second
10:22:46 child 3 started with PID = 69191, sleeping 4 second
10:22:47 wait() returned child PID = 69190, num_dead = 1
10:22:50 wait() returned child PID = 69191, num_dead = 2
10:22:53 wait() returned child PID = 69189, num_dead = 3
no more child, bye bye! 
 
 */
int
main(int argc, char *argv[])    
{
    int num_dead;
    pid_t child_pid;

    if (argc < 2 || 0 == strcmp(argv[1], "--help"))
        fprintf(stderr, "%s sleep-time...\n", argv[0]);
    // 关闭stdio缓冲区
    setbuf(stdout, NULL);

    // 创建多个子进程
    for (int i=1; i<argc; i++)
    {
        switch (fork())
        {
            case -1:
                perror("fork error");
            break;
            case 0:
                printf("%s child %d started with PID = %ld, sleeping %s second\n", curr_time("%T"), i, (long)getpid(), argv[i]);
                // 每个子进程睡眠指定时间
                sleep(getInt(argv[i], GN_NONNEG, "sleep-time"));
                _exit(EXIT_SUCCESS);
            break;
            default:
            break;
        }
    }

    num_dead = 0;

    for (;;)
    {   // 等待每个子进程睡眠结束
        child_pid = wait(NULL);

        if (-1 == child_pid)
        {
            // 子进程都已经终止
            if (ECHILD == errno)
            {
                printf("no more child, bye bye!\n");
                exit(EXIT_SUCCESS);
            }
            else
            {
                perror("wait error");
            }
        }

        num_dead++;
        printf("%s wait() returned child PID = %ld, num_dead = %ld\n", curr_time("%T"), (long)child_pid, num_dead);
    }
}

