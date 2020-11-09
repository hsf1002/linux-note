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
#include <sys/wait.h>

/**
 *   
 * 
 ./a.out 
file offset before fork: 0
O_APPEND flag before fork: off
child has existed
file offset after fork: 1000
O_APPEND flag after fork: on
 */
int
main(int argc, char *argv[])    
{
    int fd, flag;
    char template[] = "/tmp/testXXXXXX";

    // 默认行缓冲，改为不缓冲
    setbuf(stdout, NULL);

    if (-1 == (fd = mkstemp(template)))
        perror("mkstemp error");
    
    printf("file offset before fork: %lld\n", (long long)lseek(fd, 0, SEEK_CUR));

    if (-1 == (flag = fcntl(fd, F_GETFL)))
        perror("fcntl error");
    
    // O_APPEND标志默认为关闭
    printf("O_APPEND flag before fork: %s\n", (flag & O_APPEND) ? "on" : "off");

    switch (fork())
    {
    // error
    case -1:
        perror("fork error");
        break;
    // 子进程
    case 0:
        if (-1 == lseek(fd, 1000, SEEK_CUR))
            perror("lseek error");

        // 子进程将O_APPEND标志打开
        if (-1 == fcntl(fd, F_GETFL))
            perror("fcntl get error");
        flag |= O_APPEND;
        if (-1 == fcntl(fd, F_SETFL, flag))
            perror("fcntl set error");

        _exit(EXIT_SUCCESS);
    // 父进程
    default:
        // 等待子进程结束
        if (-1 == wait(NULL))
            perror("wait error");
        
        printf("child has existed\n");
        // 子进程修改了文件偏移量，父进程也同步修改
        printf("file offset after fork: %lld\n", (long long)lseek(fd, 0, SEEK_CUR));

        if (-1 == (flag = fcntl(fd, F_GETFL)))
            perror("fcntl error");
        // 子进程打开了O_APPEND，父进程受到了影响
        printf("O_APPEND flag after fork: %s\n", (flag & O_APPEND) ? "on" : "off");

        exit(EXIT_SUCCESS);
    }
}

