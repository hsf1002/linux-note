#define _GNU_SOURCE
#define _XOPEN_SOURCE
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
#include <sys/time.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <stddef.h>
#include <signal.h>
#include <ctype.h>
#include <termios.h>
#include <errno.h>
#include <poll.h>
#include "get_num.h"


/**
 * 
    使用poll检查多个文件描述符

 cc -g -Wall -o poll_pipe poll_pipe.c libgetnum.so 
./poll_pipe 10
writing (random_pipe:   1), (read fd:   5), (write fd:   6)
poll() returned: 1
readable: 1    5

 */
int main(int argc, char *argv[])
{
    int num_pipes;
    int ready;
    int random_pipe;
    int num_writes;
    int (*pfds)[2];
    struct pollfd *pollfd;

    if (argc < 2 || 0 == strcmp(argv[1], "--help"))
        fprintf(stderr, "%s num-pipes [num-writes]\n", argv[0]);
    
    num_pipes = getLong(argv[1], GN_GT_0, "num-pipes");

    if (NULL == (pfds = calloc(num_pipes, sizeof(int [2]))))
        exit(EXIT_FAILURE);

    if (NULL == (pollfd = calloc(num_pipes, sizeof(struct pollfd))))
        exit(EXIT_FAILURE);

    // 创建管道，pfds是一个数组指针
    for (int i=0; i<num_pipes; i++)
        if (-1 == pipe(pfds[i]))
            exit(EXIT_FAILURE);
    
    num_writes = (argc > 2) ? getInt(argv[2], GN_GT_0, "num-writes") : 1;

    srandom((int)time(NULL));

    // num_writes表示往多少个管道写入数据
    for (int i=0; i<num_writes; i++)
    {
        random_pipe = random() % num_pipes;

        // 打印出每个管道的读写端文件描述符
        printf("writing (random_pipe: %3d), (read fd: %3d), (write fd: %3d)\n", random_pipe, pfds[random_pipe][0], pfds[random_pipe][1]);

        // 随机向管道中写入一个字符
        if (-1 == write(pfds[random_pipe][1], "a", 1))
            perror("write error");
    }

    for (int i=0; i<num_pipes; i++)
    {   
        // 监控管道读端
        pollfd[i].fd = pfds[i][0];
        // 监控普通输入数据
        pollfd[i].events = POLLIN;
    }

    // -1表示一直阻塞，0表示不会阻塞
    if (-1 == (ready = poll(pollfd, num_pipes, -1)))
        perror("poll error");
    
    printf("poll() returned: %d\n", ready);

    for (int i=0; i<num_pipes; i++)
        // 是否有可读数据
        if (pollfd[i].revents & POLLIN)
            printf("readable: %d  %3d\n", i, pollfd[i].fd);
    
    exit(EXIT_SUCCESS);
}

