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
#include "signal_fun.h"


//#define USE_SIGSETJMP

// 是否可以跳转
static volatile sig_atomic_t can_jmp = 0;

#ifdef USE_SIGSETJMP
static sigjmp_buf env;
#else
static jmp_buf env;
#endif

/**
 * 
 * 统计捕获次数
 */
static void
handler(int sig)
{
    printf("received signal %d (%s), signal mask is: \n", sig, strsignal(sig));

    print_sigmask(stdout, NULL);

    if (!can_jmp)
    {
        printf("env buffer not yet set, doing a simple return");
        return;
    }

#ifdef USE_SIGSETJMP
    siglongjmp(env, 1);
#else
    longjmp(env, 1);
#endif    
}

/**
 *   
 * 信号处理函数中执行非本地跳转
 * 
 macOS下两种方式表现一致：
 
 1. 使用setjmp + longjmp组合: 信号处理函数调用longjmp之后信号掩码和开始保持一致

 ./sigmask_longjmp
signal mask at startup: 
		<empty signal set>
calling setjmp()
^Creceived signal 2 (Interrupt: 2), signal mask is: 
		2 (Interrupt: 2)
After jump from handler, signam mask is:
		<empty signal set>
^\Quit: 3

2. 使用sigsetjmp + siglongjmp组合: siglongjmp将信号掩码恢复到了调用sigsetjmp时候的值，即一个空信号集

./sigmask_longjmp
signal mask at startup: 
		<empty signal set>
calling sigsetjmp()
^Creceived signal 2 (Interrupt: 2), signal mask is: 
		2 (Interrupt: 2)
After jump from handler, signam mask is:
		<empty signal set>
^\Quit: 3

 */
int
main(int argc, char *argv[])    
{
    struct sigaction sa;

    print_sigmask(stdout, "signal mask at startup: \n");

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = handler;

    if (-1 == sigaction(SIGINT, &sa, NULL))
        perror("sigaction error");

#ifdef USE_SIGSETJMP
    printf("calling sigsetjmp()\n");
    if (0 == sigsetjmp(env, 1))
#else
    printf("calling setjmp()\n");
    if (0 == setjmp(env))
#endif
        can_jmp = 1;
    else
        print_sigmask(stdout, "After jump from handler, signam mask is:\n");
    
    for (;;)
        pause();

    exit(EXIT_SUCCESS);
}

