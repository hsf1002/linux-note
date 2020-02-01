#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <pthread.h>
#include <paths.h>
#include <time.h>
#include <sys/wait.h>
#include "get_num.h"


/**
 *    使用管道同步多个进程

cc -g -Wall -o pipe_sync pipe_sync.c libgetnum.so

./pipe_sync 4 2 3 5
parent started
child 2  pid=33561, closing pipe
child 3  pid=33562, closing pipe
child 1  pid=33560, closing pipe
child 4  pid=33563, closing pipe
parent ready to go
 */
int
main(int argc, char *argv[])    
{
    int fd[2];
    int dummy;

    if (argc < 2 || 0 == strcmp(argv[1], "--help"))
    {
        fprintf(stderr, "%s sleep-time\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    setbuf(stdout, NULL);

    printf("parent started \n");

    // 创建管道
    if (-1 == pipe(fd))
    {
        perror("pipe error");
        exit(EXIT_FAILURE);
    }


    for (int i=1; i<argc; i++)
    {
        switch (fork())
        {
            case -1:
                perror("fork error");
            break;
            // 子进程
            case 0:
                // 关闭读端
                if (-1 == close(fd[0]))
                    perror("child close 0 error");
                
                // 每个子进程都睡眠一段时间再退出
                sleep(getInt(argv[i], GN_NONNEG, "sleep-time"));

                printf("child %i  pid=%ld, closing pipe\n", i, getpid());

                // 关闭写端
                // 多个子进程共享管道的写端
                if (-1 == close(fd[1]))
                    perror("child close 1 error");
                _exit(EXIT_SUCCESS);
            //break;
            // 父进程
            default:
            break;
        }
    }

    // 父进程关闭写端
    if (-1 == close(fd[1]))
    {
        perror("parent close 1 error");
        exit(EXIT_FAILURE);
    }

    // 一直阻塞，只有当所有子进程的写端关闭之后，read才会返回0
    if (0 != read(fd[0], &dummy, 1))
    {
        perror("parent read 0 error");
        exit(EXIT_FAILURE);
    }

    printf("parent ready to go\n");

    exit(EXIT_SUCCESS);
}

