#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

static jmp_buf env;

static void
do_jmp(int n, int r, int v)
{
    printf("inside do_jmp(): n = %d, r = %d, v = %d\n", n, r, v);
    longjmp(env, 1);
}

/**
 *   调用longjmp对局部变量的影响

// 以常规方式编译
skydeiMac:06-进程 sky$ cc -o longjmp_vars longjmp_vars.c 
skydeiMac:06-进程 sky$ ./longjmp_vars 
inside do_jmp(): n = 777, r = 888, v = 999
after longjmp(), n = 777, r = 888, v = 999 

// 以优化方式编译后：调用longjmp后，n和r被重置为setjmp初次调用时的值，原因是优化器对代码的重组受到longjmp调用的干扰
skydeiMac:06-进程 sky$ cc -O -o longjmp_vars longjmp_vars.c 
skydeiMac:06-进程 sky$ ./longjmp_vars 
inside do_jmp(): n = 777, r = 888, v = 999
after longjmp(), n = 111, r = 222, v = 999

 */
int
main(int argc, char *argv[])    
{
    int n;
    register int r;
    // 声明为volatile，告诉编译器不要对其进行优化，从而避免了代码重组
    volatile int v;
    
    n = 111;
    r = 222;
    v = 333;

    if (0 == setjmp(env))
    {
        n = 777;
        r = 888;
        v = 999;
        do_jmp(n, r, v);
    }
    else
    {
        printf("after longjmp(), n = %d, r = %d, v = %d \n", n, r, v);
    }
    
    exit(EXIT_SUCCESS);
}

