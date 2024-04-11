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


#define USE_SIGSETJMP

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
 * cc sigmask_longjmp.c -o sigmask_longjmp libsignalinfo.so
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


------------------------------------------------------------------------------
ubuntu下快捷键：
SIGABRT：CTRL + C
SIGQUIT：CTRL + \

1. 使用setjmp + longjmp组合: 信号处理函数中添加的信号掩码，在调用longjmp之后并没有移除
./sigmask_longjmp
signal mask at startup: 
		<empty signal set>
calling setjmp()
^Creceived signal 2 (Interrupt), signal mask is: 
		2 (Interrupt)
After jump from handler, signam mask is:
		2 (Interrupt)
^C^\退出 (核心已转储)


2. 使用sigsetjmp + siglongjmp组合: 显式控制
sigsetjmp(env, 1) ------------------------不保存
/sigmask_longjmp
signal mask at startup: 
		<empty signal set>
calling sigsetjmp()
^Creceived signal 2 (Interrupt), signal mask is: 
		2 (Interrupt)
After jump from handler, signam mask is:
		<empty signal set>

^\退出 (核心已转储)

sigsetjmp(env, 0) ------------------------保存
./sigmask_longjmp
signal mask at startup: 
		<empty signal set>
calling sigsetjmp()
^Creceived signal 2 (Interrupt), signal mask is: 
		2 (Interrupt)
After jump from handler, signam mask is:
		2 (Interrupt)

 */
int
main(int argc, char *argv[])    
{
    struct sigaction sa;

    print_sigmask(stdout, "signal mask at startup: \n");

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = handler;

    // 为SIGINT注册信号处理函数，进入信号处理函数时，内核自动将引发调用的信号以及由act.sa_mask指定的任意信号添加到进程的信号掩码中
    // BSD中在处理函数正常返回时再将它们从掩码中删除；System V以及Linux中，退出信号处理函数时longjmp不会将信号掩码恢复
    if (-1 == sigaction(SIGINT, &sa, NULL))
        perror("sigaction error");

#ifdef USE_SIGSETJMP
    printf("calling sigsetjmp()\n");
    if (0 == sigsetjmp(env, 0)) // 0: 保持  1：清除
#else
    printf("calling setjmp()\n");
    // 第一次调用成功走此处
    if (0 == setjmp(env))
#endif
        can_jmp = 1;
    // 从longjmp/siglongjmp返回走此处
    else
        print_sigmask(stdout, "After jump from handler, signam mask is:\n");
    
    for (;;)
        pause();

    exit(EXIT_SUCCESS);
}

