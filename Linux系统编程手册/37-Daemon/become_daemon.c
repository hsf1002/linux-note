#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/times.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sched.h>
#include <signal.h>
#include <time.h>
#include "become_daemon.h"


/*
    变成守护进程
*/
int
become_daemon(int flag)
{
    int maxfd;
    int fd;

    // 创建子进程，并关闭父进程
    switch (fork())
    {
        case -1:
            _exit(EXIT_FAILURE);
        break;
        case 0:
            printf("pid=%ld\n", getpid());
        break;
        default:
            _exit(EXIT_SUCCESS);
    }

    // 设置文件模式屏蔽字
    if (!(flag & BD_NO_UMASK0))
        umask(0);
    
    // 更改进程根目录
    if (!(flag & BD_NO_CHDIR))
        chdir("/");
    
    // 关闭继承的所有打开的文件描述符
    if (!(flag & BD_NO_CLOSE_FILES))
    {
        if (-1 == (maxfd = sysconf(_SC_OPEN_MAX)))
            maxfd = BD_MAX_CLOSE;
        
        for (fd=0; fd<maxfd; fd++)
            close(fd);
    }

    // 无控制终端，重新打开标准输入/标准输出/标准错误，并定向到/dev/null
    if (!(flag & BD_NO_REOPEN_STD_FDS))
    {
        close(STDIN_FILENO);

        if (STDIN_FILENO != (fd = open("/dev/null", O_RDWR)))
            return -1;
        
        if (dup2(STDIN_FILENO, STDOUT_FILENO) != STDOUT_FILENO)
            return -1;

        if (dup2(STDIN_FILENO, STDERR_FILENO) != STDERR_FILENO)
            return -1;
    }
}

