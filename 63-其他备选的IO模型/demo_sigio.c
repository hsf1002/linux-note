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
#include <poll.h>
#include "tty_func.h"


static volatile sig_atomic_t got_signal = 0;

static void 
sig_io_handler(int signo)
{
    got_signal = 1;
}

/**
 * 
    终端上使用信号驱动IO

cc demo_sigio.c -o demo_sigio libtty.so
./demo_sigio
fcntl F_SETOWN error: Inappropriate ioctl for device
cnt = 185, read = a
cnt = 252, read = b
cnt = 300, read = c
cnt = 334, read = d
cnt = 384, read = e
cnt = 430, read = x
cnt = 450, read = x
cnt = 465, read = x
cnt = 660, read = #

 */
int main(int argc, char *argv[])
{
    int flag;
    int cnt;
    char ch;
    struct sigaction sa;
    struct termios orig_termios;
    bool done;

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sa.sa_handler = sig_io_handler;

    // 1. 为SIGIO建立信号处理函数，以改变其终止进程之默认行为
    if (-1 == sigaction(SIGIO, &sa, NULL))
        perror("sigaction error");
    
    // 2. 设置当前进程为标准输入的属主进程，当标准输入有IO时即通知当前进程
    if (-1 == fcntl(STDIN_FILENO, F_SETOWN, getpid()))
        perror("fcntl F_SETOWN error");

    flag = fcntl(STDIN_FILENO, F_GETFL);
    // 3. 为标准输入设置异步和非阻塞标志
    if (-1 == fcntl(STDIN_FILENO, F_SETFL, flag | O_ASYNC | O_NONBLOCK))
        perror("fcntl F_SETFL error");
    
    // 至此，调用进程可以执行其他任务
    // 设置终端为cbreak模式，这样每次输入只会有一个字符
    if (-1 == tty_set_cbreak(STDIN_FILENO, &orig_termios))
        perror("tty_set_cbreak error");
    
    // 无限循环
    for (done = false, cnt = 0; !done; cnt++)
    {
        for (int i=0; i<10000000; i++)
            continue;
        
        // 一旦收到信号标志，就读取数据，直到读完或者手动结束（按#）
        if (got_signal)
        {
            while (read(STDIN_FILENO, &ch, 1) > 0 && !done)
            {
                printf("cnt = %d, read = %c\n", cnt, ch);
                done = (ch == '#');
            }
            
            got_signal = 0;
        }
    }

    // 恢复终端属性
    if (-1 == tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios))
        perror("tcsetattr error");

    exit(EXIT_SUCCESS);
}

