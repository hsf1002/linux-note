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


/**
 *    使用管道连接ls和wc
 * 
cc -g -Wall -o pipe_ls_wc pipe_ls_wc.c 
./pipe_ls_wc 
main start
      12
main finished
ls | wc -l
      12
*/
int
main(int argc, char *argv[])    
{
    int fd[2];

    printf("main start\n");

    // 创建管道
    if (-1 == pipe(fd))
    {
        perror("pipe error");
        exit(EXIT_FAILURE);
    }

    // 处理ls的进程
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

            // 将标准输出stdout的文件描述符复制到管道的写端
            if (STDOUT_FILENO != fd[1])
            {
                if (-1 == dup2(fd[1], STDOUT_FILENO))
                    perror("dup2 error");
                if (-1 == close(fd[1]))
                    perror("child close 1 error");
            }

            // 这样ls的输出，将会写到管道中去
            execlp("ls", "ls", (char *)NULL);
            _exit(EXIT_SUCCESS);
        //break;
        // 父进程
        default:
        break;
    }
    
    // 处理wc的进程
    switch (fork())
    {
        case -1:
            perror("fork error");
        break;
        // 子进程
        case 0:
            // 关闭写端
            if (-1 == close(fd[1]))
                perror("child close 1 error");

            // 将标准输入stdin的文件描述符复制到管道的读端
            if (STDIN_FILENO != fd[0])
            {
                if (-1 == dup2(fd[0], STDIN_FILENO))
                    perror("dup2 error");
                if (-1 == close(fd[0]))
                    perror("child close 0 error");
            }

            // 这样wc的输入，将会从管道中去读
            execlp("wc", "wc", "-l", (char *)NULL);
            _exit(EXIT_SUCCESS);
        //break;
        // 父进程
        default:
        break;
    }

    // 父进程关闭读端
    if (-1 == close(fd[0]))
    {
        perror("parent close 0 error");
        exit(EXIT_FAILURE);
    }
    // 父进程关闭写端
    if (-1 == close(fd[1]))
    {
        perror("parent close 1 error");
        exit(EXIT_FAILURE);
    }

    // 等待一个子进程退出
    if (-1 == wait(NULL))
    {
        perror("wait 1 error");
        exit(EXIT_FAILURE);
    }

    // 等待另一个子进程退出
    if (-1 == wait(NULL))
    {
        perror("wait 2 error");
        exit(EXIT_FAILURE);
    }

    printf("main finished\n");

    exit(EXIT_SUCCESS);
}

