#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>

#define HAVE_ON_EXIT


static void
atexit_fun1(void)
{
    printf("atexit_fun1 called\n");
}

static void
atexit_fun2(void)
{
    printf("atexit_fun2 called\n");
}

static void
atexit_fun3(void)
{
    printf("atexit_fun3 called\n");
}

#ifdef HAVE_ON_EXIT
static void
onexit_fun1(int exit_status, void *arg)
{
    printf("onexit_fun1 called, status = %d, arg = %ld\n", exit_status, (long)arg);
}

static void
onexit_fun2(int exit_status, void *arg)
{
    printf("onexit_fun2 called, status = %d, arg = %ld\n", exit_status, (long)arg);
}
#endif

/**
 *   
 * 同时使用on_exit和atexit注册退出处理程序
onexit_fun2 called, status = 2, arg = 20
atexit_fun3 called
atexit_fun2 called
atexit_fun1 called
onexit_fun1 called, status = 2, arg = 10
*/
int
main(int argc, char *argv[])    
{
#ifdef HAVE_ON_EXIT
    if (0 != on_exit(onexit_fun1, (void *)10))
        perror("on_exit 1 error");
#endif
    if (0 != atexit(atexit_fun1))
        perror("atexit 1 error");
    if (0 != atexit(atexit_fun2))
        perror("atexit 2 error");
    if (0 != atexit(atexit_fun3))
        perror("atexit 3 error");
#ifdef HAVE_ON_EXIT
    if (0 != on_exit(onexit_fun2, (void *)20))
        perror("on_exit 2 error");   
#endif
    exit(2); 
}

