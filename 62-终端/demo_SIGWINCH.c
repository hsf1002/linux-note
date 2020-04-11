//#define _BSD_SOURCE
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



/*
    通用的信号处理函数
*/
static void
sig_handler(int signo)
{

}


/**
 * 
    监视终端窗口大小的改变

./demo_SIGWINCH
caught SIGWINCH, new window size: row = 37, colume = 128
caught SIGWINCH, new window size: row = 38, colume = 128
caught SIGWINCH, new window size: row = 39, colume = 128
caught SIGWINCH, new window size: row = 39, colume = 129
caught SIGWINCH, new window size: row = 39, colume = 130
caught SIGWINCH, new window size: row = 39, colume = 131
caught SIGWINCH, new window size: row = 39, colume = 132
caught SIGWINCH, new window size: row = 39, colume = 133
caught SIGWINCH, new window size: row = 39, colume = 134
caught SIGWINCH, new window size: row = 39, colume = 135
caught SIGWINCH, new window size: row = 39, colume = 136
caught SIGWINCH, new window size: row = 39, colume = 137
caught SIGWINCH, new window size: row = 39, colume = 138
caught SIGWINCH, new window size: row = 39, colume = 139
caught SIGWINCH, new window size: row = 39, colume = 140
caught SIGWINCH, new window size: row = 39, colume = 141
caught SIGWINCH, new window size: row = 39, colume = 142
caught SIGWINCH, new window size: row = 40, colume = 142

 */
int main(int argc, char *argv[])
{
    struct winsize ws;
    struct sigaction sa;

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = sig_handler;

    if (-1 == sigaction(SIGWINCH, &sa, NULL))
        exit(EXIT_FAILURE);
    
    for (;;)
    {
        pause();

        if (-1 == ioctl(STDIN_FILENO, TIOCGWINSZ, &ws))
            exit(EXIT_FAILURE);
        
        printf("caught SIGWINCH, new window size: row = %d, colume = %d\n", ws.ws_row, ws.ws_col);
    }
    
    exit(EXIT_SUCCESS);
}

