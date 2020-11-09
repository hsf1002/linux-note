//#define _GNU_SOURCE
//#define _XOPEN_SOURCE
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


#define max(m,n) ((m) > (n) ? (m) : (n))
static int pfd[2];

static void 
sig_handler(int signo)
{
    int save_errno;

    save_errno = errno;
    if (write(pfd[1], "x", 1) == -1 && errno == EAGAIN)
        perror("write error");
    
    errno = save_errno;
}

/**
 * 
    不使用pselect避免“等待信号同时调用select时出现的竞态条件”的另一种方案：self-pipe

skydeiMac:63-其他备选的IO模型 sky$ ./self_pipe 10

ready = 0
3:  (read end of pipe)
timeout after select(): 10.000
skydeiMac:63-其他备选的IO模型 sky$ 
skydeiMac:63-其他备选的IO模型 sky$ ./self_pipe 10
^Ca signal was caught
ready = 1
3: r (read end of pipe)
timeout after select(): 10.000
skydeiMac:63-其他备选的IO模型 sky$ ./self_pipe 10
ddd^Ca signal was caught
ready = 1
3: r (read end of pipe)
timeout after select(): 10.000

 */
int main(int argc, char *argv[])
{
    struct sigaction sa;
    int ready = -1;
    int fd;
    int nfds;
    int flag;
    char ch;
    struct timeval timeout;
    struct timeval *pto;
    fd_set readfds;

    if (argc < 2 || 0 == strcmp(argv[1], "--help"))
        fprintf(stderr, "{timeout} fd ...\n (- means infinite timeout)");

    if (0 == strcmp(argv[1], "-"))
        pto = NULL;
    else
    {
        pto = &timeout;
        timeout.tv_sec = getLong(argv[1], 0, "timeout");
        timeout.tv_usec = 0;
    }
    nfds = 0;

    FD_ZERO(&readfds);
    for (int i=2; i<argc; i++)
    {
        fd = getInt(argv[i], 0, "fd");

        if (fd >= FD_SETSIZE)
        {
            perror("fd exceeds limit");
            exit(EXIT_FAILURE);
        }

        if (fd >= nfds)
            nfds = fd + 1;
        
        FD_SET(fd, &readfds);
    }

    // 1. 创建管道
    if (-1 == pipe(pfd))
        perror("pipe error");
    
    // 2. 将读端添加到读就绪的监视列表
    FD_SET(pfd[0], &readfds);
    nfds = max(nfds, pfd[0] + 1);

    // 3. 设置读端、写端 设置为非阻塞
    if (-1 == (flag = fcntl(pfd[0], F_GETFL)))
        perror("fcntl error");
    flag |= O_NONBLOCK;
    if (-1 == (flag = fcntl(pfd[0], F_SETFL, flag)))
        perror("fcntl error");
    // 将写端设置为非阻塞的原因：防止信号到来太快，重复调用信号处理函数填满管道空间，造成信号处理函数里面的write阻塞
    if (-1 == (flag = fcntl(pfd[1], F_GETFL)))
        perror("fcntl error");
    flag |= O_NONBLOCK;
    if (-1 == (flag = fcntl(pfd[1], F_SETFL, flag)))
        perror("fcntl error");

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sa.sa_handler = sig_handler;

    // 4. 为信号安装处理函数，在创建管道之后，为了防止管道创建前就发送信号导致竞争条件
    if (-1 == sigaction(SIGINT, &sa, NULL))
        perror("sigaction error");
    
    // 5. 如果被信号中断，重新调用select，严格来说并非必须，只是表明可以通过监视readfds来检查是否有信号到来，而不是通过检查返回的errno=EINTR错误码
    while (-1 == (ready = select(nfds, &readfds, NULL, NULL, pto)) && errno == EINTR)
        continue;
    
    if (-1 == ready)
        exit(EXIT_FAILURE);

    // 读端是否位于readfds中可以判断信号是否到来了
    if (FD_ISSET(pfd[0], &readfds))
    {
        printf("a signal was caught\n");
    
        // 信号到来，读取管道中所有字节，可能会有多个信号，需要一个循环读取直到非阻塞式read返回EAGAIN错误码
        for (;;)
        {
            if (-1 == read(pfd[0], &ch, 1))
            {
                if (EAGAIN == errno)
                    break;
                else
                    exit(EXIT_FAILURE);
            }
            // 可以做出对于接收到信号的回应
        }
    }

    printf("ready = %d\n", ready);

    // 检查select返回后的文件描述符集，确定其他文件描述符是否准备就绪
    for (int i=2; i<argc; i++)
    {
        fd = getInt(argv[i], 0, "fd");
        printf("%d: %s\n", fd, FD_ISSET(fd, &readfds));
    }

    // 检查管道读端是否准备就绪
    printf("%d: %s (read end of pipe)\n", pfd[0], FD_ISSET(pfd[0], &readfds) ? "r" : "");

    // 剩余的超时时间
    if (NULL != pto)
        printf("timeout after select(): %ld.%03ld\n", (long)timeout.tv_sec, (long)timeout.tv_usec/1000);

    exit(EXIT_SUCCESS);
}

