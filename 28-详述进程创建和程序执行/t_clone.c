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
#include <signal.h>
#include <sys/types.h>
#include <sys/acct.h>
#include <sys/stat.h>
#include <time.h>
#include <sched.h>


#ifndef CHILD_SIG
// 子进程终止时产生的信号
#define CHILD_SIG SIGUSR1
#endif

/**
 * 
 */
static int
child_fun(void *arg)
{
    // 关闭之前打开的文件描述符
    if (-1 == close(*((int *)arg)))
        perror("close error");
    
    // 子进程终止
    return 0;
}

/**
 *   使用clone创建子进程
 * 
 */
int
main(int argc, char *argv[])    
{
    const int STACK_SIZE = 65536;
    char *stack;
    char *stack_top;
    int s;
    int fd;
    int flag;

    // 打开文件描述符
    if (-1 == (fd = open("/dev/null", O_RDWR)))
        perror("open error");
    
    // 父子进程是否共享文件描述符列表
    flag = (argc > 1) ? CLONE_FILES : 0;

    if (NULL == (stack = malloc(STACK_SIZE)))
        perror("malloc error");
    
    stack_top = stack + STACK_SIZE;

    // 为阻止终止子进程，忽略CHILD_SIG，为阻止产生僵尸以及无法收集子进程退出状态，不能忽略SIGCHLD
    if (0 != CHILD_SIG && SIGCHLD != CHILD_SIG)
        if (SIG_ERR == signal(CHILD_SIG, SIG_IGN))
            perror("signal error");

    // 克隆子进程
    if (-1 == clone(child_fun, stack_top, flag | CHILD_SIG, (void *)&fd))
        perror("clone error");

    // 等待子进程终止
    if (-1 == waitpid(-1, NULL, (CHILD_SIG != SIGCHLD) ? __WCLONE : 0))
        perror("waitpid error");

    // 验证：两种情况下（是否共享文件描述符）子进程关闭文件描述符时，父进程能否正常写入
    s = write(fd, "x", 1);
    if (-1 == s && EBADF == errno)
        printf("fd %d has been closed\n", fd);
    else if (-1 == s)
        printf("write on fd %d failed, unexpected error: %s\n", fd, strerror(errno));
    else
        printf("write on fd %d success\n", fd);

    exit(EXIT_SUCCESS);
}

