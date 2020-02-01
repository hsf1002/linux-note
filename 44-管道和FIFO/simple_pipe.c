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

#define BUF_SIZE 10

/**
 *    在父子进程之间使用管道进行通信

cc -g -Wall -o simple_pipe simple_pipe.c 
./simple_pipe 'it is a good day, practice makes perfect'
it is a good day, practice makes perfect
*/
int
main(int argc, char *argv[])    
{
    int fd[2];
    char buf[BUF_SIZE];
    ssize_t num_read;

    if (argc != 2 || 0 == strcmp(argv[1], "--help"))
    {
        fprintf(stderr, "%s string\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // 创建管道
    if (-1 == pipe(fd))
    {
        perror("pipe error");
        exit(EXIT_FAILURE);
    }

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
            
            for (;;)
            {
                // 从管道读取数据
                if (-1 == (num_read = read(fd[0], buf, BUF_SIZE)))
                    perror("child read error");
                if (0 == num_read)
                    break;
                // 将读取的数据输出到终端
                if (write(STDOUT_FILENO, buf, num_read) != num_read)
                    perror("child write error");
            }

            write(STDOUT_FILENO, "\n", 1);
            // 关闭读端
            if (-1 == close(fd[0]))
                perror("child close 0 error");
            _exit(EXIT_SUCCESS);
        //break;
        // 父进程
        default:
            // 关闭读端
            if (-1 == close(fd[0]))
                perror("parent close 0 error");
            // 将命令行指定内容写到管道
            if (write(fd[1], argv[1], strlen(argv[1])) != strlen(argv[1]))
                perror("parent write error");
            // 关闭写端
            if (-1 == close(fd[1]))
                perror("parent close 1 error");
            
            // 等待子进程结束
            wait(NULL);
            exit(EXIT_SUCCESS);
        //break;
    }
}

