#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <libgen.h>
#include <time.h>
#include <signal.h>
#include "get_num.h"


// 表示尚未退出子进程的数量
static volatile int num_live_child = 0;

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
 * 
 */
static void
sig_child_handler(int signo)
{
    int status;
    int save_errno;
    pid_t child_pid;

    save_errno = errno;

    printf("%s handler: caught SIGCHLD \n", curr_time("%T"));

    // 等待所有子进程退出
    while ((child_pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        printf("%s handler: reaped child %ld - ", curr_time("%T"), (long)child_pid);

        // 任意子进程终止，即打印出其退出状态
        print_wait_status(NULL, status);
        // 表示尚未退出子进程的数量
        num_live_child--;
    }

    if (child_pid == -1 && errno != ECHILD)
        perror("waitpid error");
    
    sleep(5);

    printf("%s handler: returning\n", curr_time("%T"));

    errno = save_errno;
}

/**
 *   

尽管有三个子进程退出，但是只捕获到了两个SIGCHLD信号
cc -g -Wall -o multi_SIGCHLD multi_SIGCHLD.c libgetnum.so libwaitstatus.so

./multi_SIGCHLD 1 2 4
parent PID = 18659
17:11:34 child with PID = 69905 existing
17:11:34 handler: caught SIGCHLD
17:11:34 handler: reaped child 69905 - child existed, status = 0
17:11:35 child with PID = 69906 existing
17:11:37 child with PID = 69907 existing
17:11:39 handler: returning
17:11:39 handler: caught SIGCHLD
17:11:39 handler: reaped child 69907 - child existed, status = 0
17:11:39 handler: reaped child 69906 - child existed, status = 0
17:11:44 handler: returning
17:11:44 all 3 child have terminated: SIGCHLD was caught 2 times
 */
int
main(int argc, char *argv[])    
{
    int sig_cnt;
    sigset_t block_mask;
    sigset_t empty_mask;
    struct sigaction sa;

    if (argv < 2 || 0 == strcmp(argv[1], "--help"))
        fprintf(stderr, "%s child-sleep-time\n", argv[0]);

    setbuf(stdout, NULL);
    sig_cnt = 0;
    num_live_child = argc - 1;

    // 为信号SIGCHLD注册信号处理函数
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = sig_child_handler;
    if (-1 == sigaction(SIGCHLD, &sa, NULL))
        perror("sigaction error");

    // 将信号SIGCHLD进行阻塞
    sigemptyset(&block_mask);
    sigaddset(&block_mask, SIGCHLD);
    if (-1 == sigprocmask(SIG_SETMASK, &block_mask, NULL))
        perror("sigprocmask error");

    printf("parent PID = %ld\n", (long)getppid());

    for (int i=1; i<argc; i++)
    {
        switch (fork())
        {
            case -1:
                perror("fork error");
            break;
            case 0:
                // 子进程睡眠指定时间后自动退出
                sleep(getInt(argv[i], GN_NONNEG, "child-sleep-time"));
                printf("%s child with PID = %ld existing \n", curr_time("%T"), (long)getpid());

                _exit(EXIT_SUCCESS);
            //break;
            default:
            break;
        }
    }

    sigemptyset(&empty_mask);
    // 如果还有子进程没有退出，则继续循环
    while (num_live_child > 0)
    {
        // 清除阻塞信号，暂停程序，等待接收信号，信号捕获并处理后，继续执行
        if (sigsuspend(&empty_mask) == -1 && errno != EINTR)
            perror("sigsuspend error");
        // 已经捕获的SIGCHLD的数量
        sig_cnt++;
    }

    printf("%s all %d child have terminated: SIGCHLD was caught %d times\n", curr_time("%T"), argc - 1, sig_cnt);
    
    exit(EXIT_SUCCESS);
}

