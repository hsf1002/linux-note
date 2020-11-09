#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>


// 未初始化数据段
char glob_buf[65535];
// 初始化数据段
int primes[] = {1, 3, 5, 7};

static int
square(int x)       // square的栈帧
{
    // square的栈帧上分配
    int result;

    result = x * x;
    // 返回值通过寄存器传递
    return result;
}

static void 
do_calc(int val)    // do_calc的栈帧
{
    printf("the square of %d is %d \n", val, square(val));

    if (val < 1000)
    {
        // do_calc的栈帧上分配
        int t;

        t = val * val * val;
        printf("the cube of %d is %d \n", val, t);
    }
}

/**
 * 
 */
int
main(int argc, char *argv[])    // main的栈帧
{
    // 初始化数据段
    static int key = 997;
    // 未初始化数据段
    static char buf[10240000];
    // main的栈帧上分配
    char *p;

    // 指向的内容在堆上分配
    p = malloc(1024);

    do_calc(key);

    exit(EXIT_SUCCESS);
}

