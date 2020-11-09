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

 cc -g -Wall -o sig_waitinfo t_sigwaitinfo.c libgetnum.so
// 睡眠100秒
hefeng@sw-hefeng:/home/workspace1/logs/test$ LD_LIBRARY_PATH=. ./sig_waitinfo 100 &
[1] 20389
hefeng@sw-hefeng:/home/workspace1/logs/test$ ./sig_waitinfo: pid is 20389
./sig_waitinfo: about to delay 100 seconds

// 发送第一个实时信号41
 hefeng@sw-hefeng:/home/workspace1/logs/test$ LD_LIBRARY_PATH=. ./sigqueue 20389 41 200 1
./sigqueue PID: 20393, UID: 1000
// 发送第二个实时信号40
 hefeng@sw-hefeng:/home/workspace1/logs/test$ LD_LIBRARY_PATH=. ./sigqueue 20389 40 100 1
./sigqueue PID: 20395, UID: 1000
// 发送第三个实时信号43
 hefeng@sw-hefeng:/home/workspace1/logs/test$ LD_LIBRARY_PATH=. ./sigqueue 20389 43 100 1
./sigqueue PID: 20407, UID: 1000
// 发送第四个标准信号3
 hefeng@sw-hefeng:/home/workspace1/logs/test$ LD_LIBRARY_PATH=. ./sigqueue 20389 3  10 1
./sigqueue PID: 20408, UID: 1000
// 发送第五个标准信号4
 hefeng@sw-hefeng:/home/workspace1/logs/test$ LD_LIBRARY_PATH=. ./sigqueue 20389 4  20 1
./sigqueue PID: 20411, UID: 1000
 hefeng@sw-hefeng:/home/workspace1/logs/test$ ./sig_waitinfo: finished delay
// 先接收标准信号（先发送的后接收？），再接收实时信号（接收顺序按照信号编号排序）
got signal: 4 (Illegal instruction)
    si_signo = 4, si_code = -1 (SI_QUEUE), si_value = 0
    si_pid = 20411, si_uid = 1000
got signal: 3 (Quit)
    si_signo = 3, si_code = -1 (SI_QUEUE), si_value = 0
    si_pid = 20408, si_uid = 1000
got signal: 40 (Real-time signal 6)
    si_signo = 40, si_code = -1 (SI_QUEUE), si_value = 0
    si_pid = 20395, si_uid = 1000
got signal: 41 (Real-time signal 7)
    si_signo = 41, si_code = -1 (SI_QUEUE), si_value = 0
    si_pid = 20393, si_uid = 1000
got signal: 43 (Real-time signal 9)
    si_signo = 43, si_code = -1 (SI_QUEUE), si_value = 0
    si_pid = 20407, si_uid = 1000
// 发送第六个标准信号15，终止进程
hefeng@sw-hefeng:/home/workspace1/logs/test$ LD_LIBRARY_PATH=. ./sigqueue 20389 15  15 1
./sigqueue PID: 20417, UID: 1000
 [1]+  已完成               LD_LIBRARY_PATH=. ./sig_waitinfo 100

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
        if (-1 == (sig_no = sigwaitinfo(&block_mask, &si)))
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

