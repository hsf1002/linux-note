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

// 是否收到SIGQUIT信号
static volatile sig_atomic_t got_sig_quit = 0;


/**
 * 
 * 
 */
static void
handler(int signo)
{
    printf("caught signal: %d (%s) \n", signo, strsignal(signo));

    if (signo == SIGQUIT)
        got_sig_quit = 1;
}


/**
 *   
 * 使用sigsuspend
 * 
 
LD_LIBRARY_PATH=. ./t_suspend
./t_suspend: pid is 89306
initial signal mask: 
		<empty signal set>
---------------loop_num: 1
critical section start, mask: 
		2 (Interrupt: 2)
		3 (Quit: 3)
// 第一次按下CONTROL+C        
^Cbefore suspend, pending signals: 
		2 (Interrupt: 2)
caught signal: 2 (Interrupt: 2) 
---------------loop_num: 2
critical section start, mask: 
		2 (Interrupt: 2)
		3 (Quit: 3)
// 第二次按下CONTROL+C          
^Cbefore suspend, pending signals: 
		2 (Interrupt: 2)
caught signal: 2 (Interrupt: 2) 
---------------loop_num: 3
critical section start, mask: 
		2 (Interrupt: 2)
		3 (Quit: 3)
// 第一次按下CONTROL+\          
^\before suspend, pending signals: 
		3 (Quit: 3)
caught signal: 3 (Quit: 3) 
---------Exit loop
Restored signal mask: 
		<empty signal set>
 */
int
main(int argc, char *argv[])    
{
    int loop_num;
    struct sigaction sa;
    sigset_t prev_mask;
    sigset_t block_mask;
    time_t start_time;

    printf("%s: pid is %ld\n", argv[0], (long)getpid());

    // 程序开始的信号屏蔽字
    print_sigmask(stdout, "initial signal mask: \n");

    sigemptyset(&block_mask);
    sigaddset(&block_mask, SIGINT);
    sigaddset(&block_mask, SIGQUIT);

    // 阻塞两个信号，SIGINT和SIGQUIT
    if (-1 == sigprocmask(SIG_SETMASK, &block_mask, &prev_mask))
        perror("sigprocmask error");

    sa.sa_sigaction = handler;
    sa.sa_flags = 0;
    // 清空
    sigemptyset(&sa.sa_mask);

    // 为这两个信号注册信号处理函数
    if (-1 == sigaction(SIGINT, &sa, NULL))
        perror("sigaction SIGINT error");
    if (-1 == sigaction(SIGQUIT, &sa, NULL))
        perror("sigaction SIGINT error");

    for (loop_num=1; !got_sig_quit; loop_num++)
    {
        printf("---------------loop_num: %d\n", loop_num);
        // 关键代码前的信号屏蔽字
        print_sigmask(stdout, "critical section start, mask: \n");

        // 忙等4秒钟
        for (start_time=time(NULL); time(NULL)<start_time+4;)
            continue;
        // 打印pending状态的信号
        print_sigpending(stdout, "before suspend, pending signals: \n");

        // 1. 解除信号阻塞
        // 2. 程序挂起
        // 3. 等待接收信号
        // 4. 接收到信号并调用信号处理函数
        // 5. 程序继续执行
        if (-1 == sigsuspend(&prev_mask) && errno != EINTR)
            perror("sigsuspend error");
    }

    // 恢复对信号的阻塞
    if (-1 == sigprocmask(SIG_SETMASK, &prev_mask, NULL))
        perror("sigprocmask error");
    // 程序结束后的信号屏蔽字
    print_sigmask(stdout, "---------Exit loop\nRestored signal mask: \n");

    exit(EXIT_SUCCESS);
}

