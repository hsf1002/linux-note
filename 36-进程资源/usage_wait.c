#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/times.h>
#include <sys/wait.h>
#include <sched.h>
#include <signal.h>
#include <time.h>

#define NSECS 3
#define SIG SIGUSR1

/**
 */
static void
handler(int sig)
{
    // do nothing, just interrupt sigsuspend()
    printf("got signal\n");
}

/**
 * 
 */
static void
print_child_usage(const char *msg)
{
    struct rusage r;

    printf("%s", msg);
    if (-1 == getrusage(RUSAGE_CHILDREN, &r))
        perror("getrusage error");
    
    printf("CPU time (sec):   user=%.3f, system=%.3f\n", r.ru_utime.tv_sec + r.ru_utime.tv_usec/1000000.0,
                                                         r.ru_stime.tv_sec + r.ru_stime.tv_usec/1000000.0);    
}

/**

sigemptyset start
child terminated
got signal
sigsuspend over
before wait: CPU time (sec):   user=0.000, system=0.000
after  wait: CPU time (sec):   user=1.672, system=1.328
 *    
 */
int
main(int argc, char *argv[])    
{
    clock_t start;
    sigset_t mask;
    struct sigaction sa;

    setbuf(stdout, NULL);

    // 为信号注册信号处理函数
    sa.sa_handler = handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if (-1 == sigaction(SIG, &sa, NULL))
        perror("sigaction error");
    
    // 子进程终止时通过信号通知父进程，在父进程准备好捕获之前阻塞此信号
    sigemptyset(&mask);
    sigaddset(&mask, SIG);
    if (-1 == sigprocmask(SIG_BLOCK, &mask, NULL))
        perror("sigprocmask error");
    
    switch (fork())
    {
        case -1:
            perror("fork error");
            exit(EXIT_FAILURE);

        case 0:
            // 消耗3s 的CPU 
            for (start=clock(); clock() - start < NSECS * CLOCKS_PER_SEC;)
                continue;

            printf("child terminated\n");
            // 发送信号给父进程
            if (-1 == kill(getppid(), SIG))
                perror("kill error");

            _exit(EXIT_SUCCESS);
        
        default:
            // 清空信号屏蔽字，即可以被任何信号打断
            printf("sigemptyset start\n");
            sigemptyset(&mask);
            // 等待子进程的信号
            sigsuspend(&mask);
            printf("sigsuspend over\n");

            sleep(2);

            print_child_usage("before wait: ");

            // 防止子进程变成僵尸
            if (-1 == wait(NULL))
                perror("wait error");
            
            print_child_usage("after  wait: ");
            exit(EXIT_SUCCESS);
    }
}
