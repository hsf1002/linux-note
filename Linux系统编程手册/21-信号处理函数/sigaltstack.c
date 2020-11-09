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



/**
 * 
 * 统计捕获次数
 */
static void
sigsegv_handler(int sig)
{
    int x;

    printf("caught signal %d (%s) \n", sig, strsignal(sig));
    printf("top of handler stack near     %10p\n", (void *)&x);

    // 不刷新stdio退出
    _exit(EXIT_FAILURE);
}

/**
 *   无需死循环
 */
static void
overflow_stack(int call_num)
{
    char a[100000];

    printf("call %4d - top of stack near %10p\n", call_num, &a[0]);
    overflow_stack(call_num + 1);
}

/**
 *   
 * 使用sigaltstack的使用
 * 
 
./a.out
// 标准栈的栈顶：
top of standard stack is near 0x7ffeea0e1a64
// 备用栈的栈顶：
alternate stack is at    0x105b32000 - 0x105b52fff 
call    1 - top of stack near 0x7ffeea0c9390
call    2 - top of stack near 0x7ffeea0b0cc0
call    3 - top of stack near 0x7ffeea0985f0
call    4 - top of stack near 0x7ffeea07ff20
call    5 - top of stack near 0x7ffeea067850
call    6 - top of stack near 0x7ffeea04f180
call    7 - top of stack near 0x7ffeea036ab0
call    8 - top of stack near 0x7ffeea01e3e0
call    9 - top of stack near 0x7ffeea005d10
call   10 - top of stack near 0x7ffee9fed640
call   11 - top of stack near 0x7ffee9fd4f70
call   12 - top of stack near 0x7ffee9fbc8a0
call   13 - top of stack near 0x7ffee9fa41d0
call   14 - top of stack near 0x7ffee9f8bb00
call   15 - top of stack near 0x7ffee9f73430
call   16 - top of stack near 0x7ffee9f5ad60
call   17 - top of stack near 0x7ffee9f42690
call   18 - top of stack near 0x7ffee9f29fc0
call   19 - top of stack near 0x7ffee9f118f0
call   20 - top of stack near 0x7ffee9ef9220
call   21 - top of stack near 0x7ffee9ee0b50
call   22 - top of stack near 0x7ffee9ec8480
call   23 - top of stack near 0x7ffee9eafdb0
call   24 - top of stack near 0x7ffee9e976e0
call   25 - top of stack near 0x7ffee9e7f010
call   26 - top of stack near 0x7ffee9e66940
call   27 - top of stack near 0x7ffee9e4e270
call   28 - top of stack near 0x7ffee9e35ba0
call   29 - top of stack near 0x7ffee9e1d4d0
call   30 - top of stack near 0x7ffee9e04e00
call   31 - top of stack near 0x7ffee9dec730
call   32 - top of stack near 0x7ffee9dd4060
call   33 - top of stack near 0x7ffee9dbb990
call   34 - top of stack near 0x7ffee9da32c0
call   35 - top of stack near 0x7ffee9d8abf0
call   36 - top of stack near 0x7ffee9d72520
call   37 - top of stack near 0x7ffee9d59e50
call   38 - top of stack near 0x7ffee9d41780
call   39 - top of stack near 0x7ffee9d290b0
call   40 - top of stack near 0x7ffee9d109e0
call   41 - top of stack near 0x7ffee9cf8310
call   42 - top of stack near 0x7ffee9cdfc40
call   43 - top of stack near 0x7ffee9cc7570
call   44 - top of stack near 0x7ffee9caeea0
call   45 - top of stack near 0x7ffee9c967d0
call   46 - top of stack near 0x7ffee9c7e100
call   47 - top of stack near 0x7ffee9c65a30
call   48 - top of stack near 0x7ffee9c4d360
call   49 - top of stack near 0x7ffee9c34c90
call   50 - top of stack near 0x7ffee9c1c5c0
call   51 - top of stack near 0x7ffee9c03ef0
call   52 - top of stack near 0x7ffee9beb820
call   53 - top of stack near 0x7ffee9bd3150
call   54 - top of stack near 0x7ffee9bbaa80
call   55 - top of stack near 0x7ffee9ba23b0
call   56 - top of stack near 0x7ffee9b89ce0
call   57 - top of stack near 0x7ffee9b71610
call   58 - top of stack near 0x7ffee9b58f40
call   59 - top of stack near 0x7ffee9b40870
call   60 - top of stack near 0x7ffee9b281a0
call   61 - top of stack near 0x7ffee9b0fad0
call   62 - top of stack near 0x7ffee9af7400
call   63 - top of stack near 0x7ffee9aded30
call   64 - top of stack near 0x7ffee9ac6660
call   65 - top of stack near 0x7ffee9aadf90
call   66 - top of stack near 0x7ffee9a958c0
call   67 - top of stack near 0x7ffee9a7d1f0
call   68 - top of stack near 0x7ffee9a64b20
call   69 - top of stack near 0x7ffee9a4c450
call   70 - top of stack near 0x7ffee9a33d80
call   71 - top of stack near 0x7ffee9a1b6b0
call   72 - top of stack near 0x7ffee9a02fe0
call   73 - top of stack near 0x7ffee99ea910
call   74 - top of stack near 0x7ffee99d2240
call   75 - top of stack near 0x7ffee99b9b70
call   76 - top of stack near 0x7ffee99a14a0
call   77 - top of stack near 0x7ffee9988dd0
call   78 - top of stack near 0x7ffee9970700
call   79 - top of stack near 0x7ffee9958030
call   80 - top of stack near 0x7ffee993f960
call   81 - top of stack near 0x7ffee9927290
call   82 - top of stack near 0x7ffee990ebc0
call   83 - top of stack near 0x7ffee98f64f0
caught signal 11 (Segmentation fault: 11)
// 信号处理函数的栈顶：
top of handler stack near     0x105b51a98

 */
int
main(int argc, char *argv[])    
{
    stack_t sigstack;
    struct sigaction sa;
    int j;

    // 开始栈顶的位置：
    printf("top of standard stack is near %10p\n", (void *)&j);

    if (NULL == (sigstack.ss_sp = (char *)malloc(SIGSTKSZ)))
        perror("malloc error");
    
    sigstack.ss_size = SIGSTKSZ;
    sigstack.ss_flags = 0;

    // 创建备用栈
    if (-1 == sigaltstack(&sigstack, NULL))
        perror("sigaltstack error");
    
    printf("alternate stack is at    %10p - %p \n", sigstack.ss_sp, (char *)sbrk(0) - 1);

    sa.sa_handler = sigsegv_handler;
    sigemptyset(&sa.sa_mask);
    // handler 使用备用栈
    sa.sa_flags = SA_ONSTACK;

    if (-1 == sigaction(SIGSEGV, &sa, NULL))
        perror("sigaction error");

    overflow_stack(1);

    exit(EXIT_SUCCESS);
}

