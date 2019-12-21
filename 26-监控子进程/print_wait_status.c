
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/wait.h>


/**
 * 
 * 打印进程终止的状态
 */
void print_wait_status(const char *msg, int status)
{
    if (NULL != msg)
        printf("%s", msg);
    
    // 正常终止
    if (WIFEXITED(status))
        printf("child existed, status = %d\n", WEXITSTATUS(status));
    // 信号终止
    else if (WIFSIGNALED(status))
    {
        printf("child killed by signal, %d (%s)\n", WTERMSIG(status), strsignal(WTERMSIG(status)));

        #ifdef WCOREDUMP 
            if (WCOREDUMP(status))
                printf("core dumped \n");
        #endif
    }
    // 信号停止
    else if (WIFSTOPPED(status))
    {
        printf("child stopped by signal %d (%s)\n", WSTOPSIG(status), strsignal(WSTOPSIG(status)));
    }
    // 信号恢复
#ifdef WIFCONTINUED
    else if (WIFCONTINUED(status))
        printf("child continued\n");
#endif
    // 不应该走到这里
    else
    {
        printf("what happend to this child? status = %x\n", (unsigned int)status);
    }
}
