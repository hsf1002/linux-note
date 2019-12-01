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
#include "get_num.h"
#include "signal_fun.h"

static int sig_cnt[NSIG];

static volatile sig_atomic_t got_sigint = 0;

/**
 * 
 * SIGINT信号，设置标志变量，其他信号统计捕获次数
 */
static void
handler(int sig)
{
    if (SIGINT == sig)
        got_sigint = 1;
    else    
        sig_cnt[sig]++;
}


/**
 *   
 * 捕获信号并统计次数

SIGUSR1: 10  SIGINT: 2

skydeiMac:20-信号基本概念 sky$ ./sig_receiver 15 &
[1] 68487
skydeiMac:20-信号基本概念 sky$ ./sig_receiver: PID is 68487 
./sig_receiver: sleeping for 15 seconds
./sig_sender: sending signal 10 to process 68487 1000000 times
./sig_receiver: pending signals are: 
		10 (Bus error: 10)
./sig_sender: existing
./sig_receiver: signal 10 caught 29538 times 
[1]+  Done                    ./sig_receiver 15


*/
int
main(int argc, char *argv[])    
{
    int n;
    int num_sec;
    sigset_t pending_mask;
    sigset_t blocking_mask;
    sigset_t empty_mask;

    printf("%s: PID is %ld \n", argv[0], (long)getpid());

    // 对每个信号都注册同一个信号处理函数
    for (n=1; n<NSIG; ++n)
        (void)signal(n, handler);

    if (argc > 1)
    {
        num_sec = getInt(argv[1], GN_GT_0, NULL);
        // 将所有信号都添加到信号集，除了SIGKILL和SIGSTOP都可以捕获
        sigfillset(&blocking_mask);
        if (-1 == sigprocmask(SIG_SETMASK, &blocking_mask, NULL))
            perror("sigprocmask error");
        
        printf("%s: sleeping for %d seconds\n", argv[0], num_sec);
    
        // 睡眠一段时间，会暂时阻塞所有信号
        sleep(num_sec);

        // 获取所有阻塞的信号并打印出来
        if (-1 == sigpending(&pending_mask))
            perror("sigpending error");
        
        printf("%s: pending signals are: \n", argv[0]);
        print_sigset(stdout, "\t\t", &pending_mask);

        // 解除对所有信号的阻塞（将信号掩码设置为空集）
        sigemptyset(&empty_mask);
        if (-1 == (sigprocmask(SIG_SETMASK, &empty_mask, NULL)))
            perror("sigprocmask error");
    }

    // 收到SIGINT才会退出
    while (!got_sigint)
        continue;

    // 将所有捕获到的信号次数打印出来
    for (n=1; n<NSIG; n++)
        if (0 != sig_cnt[n])
            printf("%s: signal %d caught %d time%s \n", argv[0], n, sig_cnt[n], (sig_cnt[n] == 1) ? "" : "s");

    exit(EXIT_SUCCESS);
}

