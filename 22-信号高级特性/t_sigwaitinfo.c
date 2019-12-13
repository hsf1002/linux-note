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



/**
 *   
 * 使用sigsuspend的替代方案sigwaitinfo
 * 
 * 
./t_sigwaitinfo 60 &
./sigqueue pid 43 100
./sigqueue pid 42 200


等待接收到信号后：
echo $$ // display PID of shell
kill -USR1 pid
kill %1

 */
int
main(int argc, char *argv[])    
{
    int loop_num;
    struct sigaction sa;
    siginfo_t si;
    sigset_t block_mask;
    int sig_no;

    if (argc > 1 && 0 == strcmp(argv[1], "--help"))
        fprintf(stderr, "%s [delay-seconds]\n", argv[0]);

    printf("%s: pid is %ld\n", argv[0], (long)getpid());

    sigfillset(&block_mask);

    // 阻塞所有信号，除了SIGKILL和SIGSTOP
    if (-1 == sigprocmask(SIG_SETMASK, &block_mask, NULL))
        perror("sigprocmask error");

    if (argc > 1)
    {
        printf("%s: about to delay %s seconds\n", argv[0], argv[1]);
        // 休眠，从而允许在调用sigwaitinfo之前发送信号，睡眠结束后循环接收排队信号
        sleep(getInt(argv[1], GN_GT_0, "deley_secs"));
        printf("%s: finished delay\n", argv[0]);
    }

    for (;;)
    {
        // 程序挂起，等待信号到达
        if (-1 == sigwaitinfo(&block_mask, &si))
            perror("sigwaitinfo error");

        // 接收到信号，程序继续执行
        // 如果接收的是这两个信号，终止进程
        if (SIGINT == sig_no || SIGTERM == sig_no)
            exit(EXIT_SUCCESS);

        // 否则打印出接收到的信号信息
        printf("got signal: %d (%s)\n", sig_no, strsignal(sig_no));
        printf("    si_signo = %d, si_code = %d (%s), si_value = %d\n", si.si_signo, si.si_code, \
            (si.si_code == SI_USER) ? "SI_USER" : (si.si_code == SI_QUEUE) ? "SI_QUEUE" : "other");
        printf("    si_pid = %ld, si_uid = %ld\n", (long)si.si_pid, (long)si.si_uid);
    }

    exit(EXIT_SUCCESS);
}

