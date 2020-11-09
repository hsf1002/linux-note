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


/**
 * 
 */
static void
sig_hanlder(int sig)
{
    static int count = 0;

    // 仅仅返回
    if (SIGINT == sig)
    {
        count++;
        printf("caught SIGINT %d \n", count);
        return;
    }

    // 终止进程
    printf("caught SIGQUIT - that's all folks!\n");
    exit(EXIT_SUCCESS);
}

/**
 *
./a.out 
^Ccaught SIGINT 1 
^Ccaught SIGINT 2 
^Ccaught SIGINT 3 
^Ccaught SIGINT 4 
^Ccaught SIGINT 5 
ç^Ccaught SIGINT 6 
^Ccaught SIGINT 7 
^\caught SIGQUIT - that's all folks!

 * 为不同信号建立同一信号处理函数
 */
int
main(int argc, char *argv[])    
{
    if (SIG_ERR == signal(SIGINT, sig_hanlder))
        perror("signal SIGINT error");
    if (SIG_ERR == signal(SIGQUIT, sig_hanlder))
        perror("signal SIGQUIT error");

    for (;;)
        pause();

    exit(EXIT_SUCCESS);
}

