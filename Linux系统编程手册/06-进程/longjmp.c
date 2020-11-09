#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

static jmp_buf env;

static void
f2(void)
{
    longjmp(env, 2);
}

static void
f1(int argc)
{
    if (argc == 1)
        longjmp(env, 1);
    f2();
}

/**
 *  
skydeiMac:06-进程 sky$ ./a.out 
calling f1() after initial setjmp()
we jumped back from f1()
skydeiMac:06-进程 sky$ ./a.out x
calling f1() after initial setjmp()
we jumped back from f2()

*/
int
main(int argc, char *argv[])    
{
    // 设置跳转目标，即每次调用longjmp总会跳转到此处
    switch (setjmp(env))
    {
        // 第一次调用setjmp应该返回0
        case 0:
        {
            printf("calling f1() after initial setjmp()\n");
            f1(argc);
        }
        break;
        case 1:
        {
            printf("we jumped back from f1()\n");
        }
        break;
        case 2:
        {
            printf("we jumped back from f2()\n");
        }
        break;
        default:
            break;
    }

    exit(EXIT_SUCCESS);
}

