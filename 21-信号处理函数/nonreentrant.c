#define _XOPEN_SOURCE 600
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


static char *str2; // set by argv[2]
static int handled = 0;

/**
 * 
 * 统计捕获次数
 */
static void
handler(int sig)
{
    crypt(str2, "xx");
    handled++;
}

/**
 *   
 * main和信号处理函数中调用不可重入函数


./a.out abc def
^Cmismatch on call 1741670 (mismatch = 1 handled = 1)
^Cmismatch on call 2402283 (mismatch = 2 handled = 2)
^Cmismatch on call 3454500 (mismatch = 3 handled = 3)
^Cmismatch on call 4085952 (mismatch = 4 handled = 4)
^Cmismatch on call 5126069 (mismatch = 5 handled = 5)
^Cmismatch on call 8361062 (mismatch = 6 handled = 6)
^Cmismatch on call 8597063 (mismatch = 7 handled = 7)
^Cmismatch on call 8816863 (mismatch = 8 handled = 8)
^\Quit: 3

*/
int
main(int argc, char *argv[])    
{
    char *cr1;
    int num_call;
    int mismatch;
    struct sigaction sa;

    if (argc != 3)
        fprintf(stderr, "%s str1 str2\n", argv[0]);

    // 由argv[2]设置
    str2 = argv[2];
    // 是argv[1]加密后复制的字符串，crypt返回静态内存，属于不可重入函数
    if (NULL == (cr1 = strdup(crypt(argv[1], "xx"))))
        perror("strdup error");

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = handler;

    if (-1 == sigaction(SIGINT, &sa, NULL))
        perror("sigaction error");

    for (num_call=1, mismatch=0; ; num_call++)
    {
        // 在不产生信号的情况下，结果总是匹配，一旦收到SIGINT信号，而主程序又恰好在crypt调用之后，字符串的匹配检查之前
        // 遭遇信号处理函数的中断，就会发生字符串不匹配的情况
        if (0 != strcmp(crypt(argv[1], "xx"), cr1))
        {
            mismatch++;
            printf("mismatch on call %d (mismatch = %d handled = %d)\n", num_call, mismatch, handled);
        }
    }
    
    exit(EXIT_SUCCESS);
}

