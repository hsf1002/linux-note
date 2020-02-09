//#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <stddef.h>
#include <signal.h>
#include <ctype.h>
#include "region_locking.h"
#include "create_pid_file.h"


#define BUF_SIZE 100


/**
 * 需要创建单例的程序如守护进程，可以调用该接口
 * 
 */
int
create_pid_file(const char* prog_name, const char *pid_file, int flag)
{
    int fd;
    char buf[BUF_SIZE];

    if (-1 == (fd = open(pid_file, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR)))
    {
        perror("open error");
        exit(EXIT_FAILURE);
    }

    // 添加FD_CLOEXEC标记
    if (flag & CPE_CLOEXEC)
    {
        if (-1 == (flag = fcntl(fd, F_GETFD)))
        {
            perror("fcntl F_GETFD error");
            exit(EXIT_FAILURE);
        }

        flag |= FD_CLOEXEC;

        if (-1 == fcntl(fd, F_SETFD, flag))
        {
            perror("fcntl F_SETFD error");
            exit(EXIT_FAILURE);
        }
    }

    // 对整个文件加写锁
    if (-1 == lock_region(fd, F_WRLCK, SEEK_SET, 0, 0))
    {
        if (errno == EAGAIN || errno == EACCES)
        {
            fprintf(stderr, "PID file %s is locked, %s probaly already running\n", pid_file, prog_name);
            exit(EXIT_FAILURE);
        }
        else
        {
            fprintf(stderr, "unable to lock PID file %s\n", pid_file);
        }
    }

    // 加锁成功后将文件内容清空
    if (-1 == ftruncate(fd, 0))
    {
        perror("ftruncate to 0 error");
        exit(EXIT_FAILURE);
    }

    snprintf(buf, BUF_SIZE, "%ld\n", (long)getpid());
    // 将pid写入文件
    if (write(fd, buf, strlen(buf)) != strlen(buf))
    {
        perror("write error");
        exit(EXIT_FAILURE);
    }

    return fd;
}
