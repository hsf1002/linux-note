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
#include <sys/time.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <stddef.h>
#include <signal.h>
#include <ctype.h>
#include <termios.h>
#include <errno.h>
#include "get_num.h"


/*
    
*/
static void
usage_error(const char *progName)
{
    fprintf(stderr, "Usage: %s {timeout|-} fd-num[rw]...\n", progName);
    fprintf(stderr, "    - means infinite timeout; \n");
    fprintf(stderr, "    r = monitor for read\n");
    fprintf(stderr, "    w = monitor for write\n\n");
    fprintf(stderr, "    e.g.: %s - 0rw 1w\n", progName);

    exit(EXIT_FAILURE);
}


/**
 * 
    使用select检查多个文件描述符

skydeiMac:63-其他备选的IO模型 sky$ ./t_select 10 0r
// 按回车，表示标准输入有可用数据
nfds = 1, ready = 1
0: r
timeout after select(): 10.000 // macOS下剩余的超时时间一直不变，即使等了几秒，返回依然是10
skydeiMac:63-其他备选的IO模型 sky$
skydeiMac:63-其他备选的IO模型 sky$ ./t_select 0 0r
nfds = 1, ready = 0
0:
timeout after select(): 0.000
// 不设置超时，同时监控标准输入和标准输出，会立刻返回，表明标准输出有数据了
skydeiMac:63-其他备选的IO模型 sky$ ./t_select - 0r 1w
nfds = 2, ready = 1
0:
1: w

 */
int main(int argc, char *argv[])
{
    fd_set read_fds;
    fd_set write_fds;
    int ready;
    int nfds;
    int fd;
    int num_read;
    struct timeval timeout;
    struct timeval *pto;
    char buf[10]; // hold "rw\0"

    if (argc < 2 || 0 == strcmp(argv[1], "--help"))
        usage_error(argv[0]);
    
    // 指定超时时间
    if (0 == strcmp(argv[1], "-"))
    {
        pto = NULL;
    }
    else
    {
        pto = &timeout;
        timeout.tv_sec = getLong(argv[1], 0, "timeout");
        timeout.tv_usec = 0;
    }

    nfds = 0;
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);

    // 命令行添加读或写的待监控的文件描述符
    for (int i=2; i<argc; i++)
    {
        num_read = sscanf(argv[i], "%d%2[rw]", &fd, buf);

        if (2 != num_read)
            usage_error(argv[0]);
        
        if (fd >= FD_SETSIZE)
            fprintf(stderr, "file descriptor exceeds limit (%d)\n", FD_SETSIZE);
        
        if (fd >= nfds)
            nfds = fd + 1;
        if (NULL != strchr(buf, 'r'))
            FD_SET(fd, &read_fds);
        if (NULL != strchr(buf, 'w'))
            FD_SET(fd, &write_fds);
    }

    // 根据pto一直阻塞，或阻塞pto的时间
    if (-1 == (ready = select(nfds, &read_fds, &write_fds, NULL, pto)))
        perror("select error");

    printf("nfds = %d, ready = %d\n", nfds, ready);
    
    // 依次检查每个文件描述符是否就绪
    for (fd=0; fd<nfds; fd++)
    {
        printf("%d: %s%s\n", fd, FD_ISSET(fd, &read_fds) ? "r" : "", FD_ISSET(fd, &write_fds) ? "w" : "");
    }

    if (NULL != pto)
    {
        printf("timeout after select(): %ld.%03ld\n", (long)timeout.tv_sec, (long)timeout.tv_usec / 100000);
    }
    
    exit(EXIT_SUCCESS);
}

