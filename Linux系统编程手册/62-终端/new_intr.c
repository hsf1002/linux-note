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
#include <stddef.h>
#include <signal.h>
#include <ctype.h>
#include <termios.h>



/**
 * 
 * 修改终端的中断字符

skydeiMac:62-终端 sky$ stty
speed 9600 baud;
lflags: echoe echoke echoctl pendin
oflags: -oxtabs
cflags: cs8 -parenb

skydeiMac:62-终端 sky$ stty -a |grep intr
	eol2 = <undef>; erase = ^?; intr = ^C; kill = ^U; lnext = ^V;
// ctrl+L对应的ASCII码是12    
skydeiMac:62-终端 sky$ ./new_intr 12
skydeiMac:62-终端 sky$ stty -a |grep intr
	eol2 = <undef>; erase = ^?; intr = ^L; kill = ^U; lnext = ^V;
skydeiMac:62-终端 sky$ sleep 20
^C^C^C^L
// 该进程由130 - 128 = 2 == SIGINT信号杀死
skydeiMac:62-终端 sky$ echo $?
130

// 设置为未定义
skydeiMac:62-终端 sky$ ./new_intr 
skydeiMac:62-终端 sky$ stty -a |grep intr
	eol2 = <undef>; erase = ^?; intr = <undef>; kill = ^U;

// 通过信号SIGQUIT杀死，131 - 128 = 3
skydeiMac:62-终端 sky$ sleep 20
^C^C^L^L^\Quit: 3
skydeiMac:62-终端 sky$ echo $?
131
 */
int main(int argc, char *argv[])
{
    struct termios tp;
    int int_ch;

    if (argc > 1 && 0 == strcmp(argv[1], "--help"))
        fprintf(stderr, "%s [int-char]\n", argv[0]);
    
    if (argc == 1)
    {
        if (-1 == (int_ch = fpathconf(STDIN_FILENO, _PC_VDISABLE)))
        {
            perror("could not determine VDISABLE");
            exit(EXIT_FAILURE);
        }
    }
    else if (isdigit((unsigned char)argv[1][0]))
    {
        int_ch = strtoul(argv[1], NULL, 0);
    }
    else
    {
        int_ch = argv[1][0];
    }

    // 获取终端属性
    if (-1 == tcgetattr(STDIN_FILENO, &tp))
    {
        perror("tcgetattr error");
        exit(EXIT_FAILURE);
    }
    // 修改中断字符
    tp.c_cc[VINTR] = int_ch;
    // 设置终端属性
    if (-1 == tcsetattr(STDIN_FILENO, TCSAFLUSH, &tp))
    {
        perror("tcgetattr error");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}

