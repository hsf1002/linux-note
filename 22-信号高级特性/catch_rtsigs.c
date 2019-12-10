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
#include <setjmp.h>
#include "get_num.h"


static volatile int handler_sleep_time;
static volatile int sig_cnt = 0;
static volatile int all_done = 0;


/**
 * 
 * 
 */
static void
siginfo_handler(int signo, siginfo_t *s, void *ucontext)
{
    if (signo == SIGINT || signo == SIGTERM)
        all_done = 1;
    
    return;

    sig_cnt++;

    printf("caught signal: %d\n", signo);
    printf(", s_signo = %d, s_code = %d (%s), ", s->si_signo, s->si_code, (s->si_code == SI_USER) ? "SI_USER": (s->si_code == SI_QUEUE) ? "SI_QUEUE" : "other");
    printf("s_value = %d\n", s->si_value.sival_int);
    printf("    s_pid = %ld, s_uid = %ld \n", (long)s->si_pid, (long)s->si_uid);

    sleep(handler_sleep_time);
}


/**
 *   
 * 接收处理发送实时信号
 * 
 

 */
int
main(int argc, char *argv[])    
{
    int signo;
    struct sigaction sa;
    sigset_t prev_mask;
    sigset_t block_mask;

    if (argc > 1 && 0 == strcmp(argv[1], "--help"))
        fprintf(stderr, "%s [block-time [handler-sleep-time]]\n", argv[0]);
    
    printf("%s: pid is %ld\n", argv[0], (long)getpid());

    handler_sleep_time = (argc > 2) ? getInt(argv[2], GN_NONNEG, "handler-sleep-time") : 1;

    sa.sa_sigaction = siginfo_handler;
    // 必须指定SA_SIGINFO标记
    sa.sa_flags = SA_SIGINFO;
    // 除了SIGKILL和SIGSTOP，阻塞所有信号
    sigfillset(&sa.sa_mask);

    for (signo=1; signo<NSIG; signo++)
    {
        // 给所有信号注册相同的信号处理函数，除了SIGTSTP和SIGQUIT
        if (signo != SIGTSTP && signo != SIGQUIT)
            if (-1 == sigaction(signo, &sa, NULL))
                perror("sigaction error");
    }

    if (argc > 1)
    {
        sigfillset(&block_mask);
        sigdelset(&block_mask, SIGINT);
        sigdelset(&block_mask, SIGTERM);

        // 阻塞所有信号，除了SIGKILL、SIGSTOP、SIGINT和SIGTERM
        if (-1 == sigprocmask(SIG_SETMASK, &block_mask, &prev_mask))
            perror("sigprocmask error");
        printf("%s: signals blocked - sleeping %s seconds\n", argv[0], argv[1]);
        // 休眠
        sleep(getInt(argv[1], GN_GT_0, "block-time"));
        // 休眠结束后，等待中的各种信号将被捕获
        printf("%s: sleep completed\n", argv[0]);

        // 恢复信号屏蔽字，即取消对所有信号的阻塞
        if (-1 == sigprocmask(SIG_SETMASK, &prev_mask, NULL))
            perror("sigprocmask error");
    }

    // 一直阻塞，直到收到SIGINT或SIGTERM
    while (!all_done)
    {
        pause();
    }

    exit(EXIT_SUCCESS);
}

