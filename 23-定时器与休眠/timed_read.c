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
#include "get_num.h"


/**
 * 
 */
static void
handler(int signo)
{
    printf("caught signal \n");
}


/**
 *   
 * 
 * 为阻塞调用read设置超时
 * 

./timed_read 5
caught signal 
111
222
333
444
num = -1
errno = 4
read timeout: Interrupted system call

 */
int
main(int argc, char *argv[])    
{
    struct sigaction sa;
    char buf[BUFSIZ];
    ssize_t num_read;
    int saved_errno;

    if (argc > 1 && 0 == strcmp(argv[1], "--help"))
        fprintf(stderr, "%s [num-sec [restart-flag]] \n", argv[0]);
    
    // 设定SA_RESTART
    sa.sa_flags = (argc > 2) ? SA_RESTART : 0;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = handler;

    // 注册信号处理函数
    if (-1 == sigaction(SIGALRM, &sa, NULL))
        perror("sigaction error");
    
    // 设置定时器
    alarm((argc > 1) ? getInt(argv[1], GN_NONNEG, "num-sec") : 10);

    // 阻塞在此，如果超时，将先调用信号处理函数，并中断此系统调用
    num_read = read(STDIN_FILENO, buf, BUFSIZ - 1);
    printf("111\n");
    // 保存errno
    saved_errno = errno;
    printf("222\n");
    // 取消定时器
    alarm(0);
    printf("333\n");
    // 恢复errno
    errno = saved_errno;
    printf("444\n");
    printf("num = %d\n", num_read);
    printf("errno = %d\n", errno);

    if (-1 == num_read)
        // 超时引起的中断read
        if (EINTR == errno)
            perror("read timeout");
        // read出错
        else
            perror("read error");
    else
        // read成功
        printf("successfull read (%ld bytes): %.*s", (long)num_read, (int)num_read, buf);

    exit(EXIT_SUCCESS);
}

