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
#include "signal_fun.h"


/**
 * 统计信号集中信号的个数
 */
void
print_sigset(FILE *of, const char *prefix, const sigset_t *sigset)
{
    int sig = 0;
    int cnt = 0;

    for (sig=1; sig<NSIG; ++sig)
    {
        if (sigismember(sigset, sig))
        {
            cnt++;
            // 将前缀、信号编号、信号描述保存到文件
            fprintf(of, "%s%d (%s)\n", prefix, sig, strsignal(sig));
        }
    }

    if (0 == cnt)
        fprintf(of, "%s<empty signal set>\n", prefix);
}

/**
 * 
 * 统计处于阻塞状态的信号个数
 */
int
print_sigmask(FILE *of, const char *msg)
{
    sigset_t curr_mask;

    if (NULL != msg)
        fprintf(of, "%s", msg);
    
    if (-1 == sigprocmask(SIG_BLOCK, NULL, &curr_mask))
        return -1;

    print_sigset(of, "\t\t", &curr_mask);
}

/**
 * 
 * 统计处于pending状态的信号个数
 */
int 
print_sigpending(FILE *of, const char *msg)
{
    sigset_t sig_pending;

    if (NULL != msg)
        fprintf(of, "%s", msg);
    
    if (-1 == sigpending(&sig_pending))
        return -1;
    
    print_sigset(of, "\t\t", &sig_pending);
}
