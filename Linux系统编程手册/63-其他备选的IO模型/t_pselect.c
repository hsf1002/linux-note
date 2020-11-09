#define _GNU_SOURCE
#define _XOPEN_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <stddef.h>
#include <signal.h>
#include <ctype.h>
#include <termios.h>
#include <errno.h>


static volatile sig_atomic_t got_signal = 0;

static void 
sig_handler(int signo)
{
    got_signal = 1;
}

/**
 * 
    终端上使用pselect


 */
int main(int argc, char *argv[])
{
    sigset_t empty_set;
    sigset_t block_set;
    struct sigaction sa;
    int ready = -1;
    int nfds;
    fd_set readfds;

    sigemptyset(&block_set);
    sigaddset(&block_set, SIGUSR1);

    // 先阻塞信号 SIGUSR1
    if (-1 == sigprocmask(SIG_BLOCK, &block_set, NULL))
        perror("sigprocmask error");

    sa.sa_flags = SA_RESTART;
    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);

    // 为SIGUSR1建立信号处理函数
    if (-1 == sigaction(SIGUSR1, &sa, NULL))
        perror("sigaction error");
    
    sigemptyset(&empty_set);
    // 1. 设置信号屏蔽字为空  2. 等IO就绪之后  3. 恢复信号屏蔽字
    if (-1 == (ready = pselect(nfds, &readfds, NULL, NULL, NULL, &empty_set)))
        perror("pselect error");
    
    exit(EXIT_SUCCESS);
}

