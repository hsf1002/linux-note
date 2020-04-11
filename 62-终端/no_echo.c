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


#define BUF_SIZE  100

/**
 * 
 * 关闭终端回显功能

cc no_echo.c -o no_echo

./no_echo 
Enter text: 
Read: hello sky

 */
int main(int argc, char *argv[])
{
    struct termios tp, save;
    char buf[BUF_SIZE];

    // 获取终端属性
    if (-1 == tcgetattr(STDIN_FILENO, &tp))
    {
        perror("tcgetattr error");
        exit(EXIT_FAILURE);
    }
    save = tp;
    // 关闭终端回显功能
    tp.c_lflag &= ~ECHO;
    // 设置终端属性
    if (-1 == tcsetattr(STDIN_FILENO, TCSAFLUSH, &tp))
    {
        perror("tcgetattr error");
        exit(EXIT_FAILURE);
    }

    printf("Enter text: ");
    fflush(stdout);

    if (NULL == fgets(buf, BUF_SIZE, stdin))
        printf("Got end-of-file/error\n");
    else
        printf("\nRead: %s", buf);
    
    // 设置终端属性
    if (-1 == tcsetattr(STDIN_FILENO, TCSAFLUSH, &save))
    {
        perror("tcgetattr error");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}

