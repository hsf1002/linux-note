#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <signal.h>
#include <netdb.h>

#define INT_LEN  30
#define PORT_NUM "50000"
#define BACKLOG  50

#define ADDRSTRLEN  (NI_MAXHOST + NI_MAXSERV + 10)


/*
    一次读取一行
 */
ssize_t
readLine(int fd, void *buffer, size_t n)
{
    ssize_t numRead;
    size_t toRead;
    char *buf;
    char ch;

    if (n <= 0 || buffer == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    buf = buffer;
    toRead = 0;

    for (;;)
    {
        // 一次读取一个字节
        numRead = read(fd, &ch, 1);

        // 读取失败
        if (numRead == -1)
        {
            if (errno == EINTR)
                continue;
            else
                return -1;
        }
        // 遇到EOF
        else if (numRead == 0)
        {
            // 没有读取到数据，之间返回
            if (toRead == 0)
                return 0;
            // 读取到了数据，添加'\0'
            else
                break;
        }
        else
        {
            // 没有读取完毕，继续读取
            if (toRead < n - 1)
            {
                toRead++;
                *buf++ = ch;
            }
            // 如果遇到换行符，停止
            if (ch == '\n')
                break;
        }
    }

    *buf = '\0';
    return toRead;
}

/**
 * Internet domain流socket客户端

./stream_server &
[3] 18283
./stream_client localhost
sequence number: 0
./stream_client localhost 10
sequence number: 1
./stream_client localhost
sequence number: 11
*/
int 
main(int argc, char **argv)
{
    char *reqLenStr;
    char seqNumStr[INT_LEN];
    int cfd;
    ssize_t numRead;
    struct addrinfo hints;
    struct addrinfo *result, *rp;

    if (argc < 2 || strcmp(argv[1], "--help") == 0)
        fprintf(stderr, "%s server-host[seq-len] \n", argv[0]);

    memset(&hints, 0x00, sizeof(struct addrinfo));
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;    // IPv4 & IPv6
    hints.ai_flags = AI_NUMERICSERV;

    if (0 != getaddrinfo(argv[1], PORT_NUM, &hints, &result))
        perror("getaddrinfo error"); 

    // 轮询该地址列表
    for (rp=result; rp!=NULL; rp=rp->ai_next)
    {
        // 创建
        if (-1 == (cfd = (socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol))))
            continue;
        
        // 连接成功
        if (-1 != connect(cfd, rp->ai_addr, rp->ai_addrlen))
            break;
        // 连接失败，尝试下一个地址
        close(cfd);
    }

    if (rp == NULL)
    {
        perror("connect error");
        return -1;
    }
    
    // 释放
    freeaddrinfo(result);

    // 默认序列号是1
    reqLenStr = (argc > 2) ? argv[2] : "1";

    // 把序列号发送到服务端
    if (write(cfd, reqLenStr, strlen(reqLenStr)) != strlen(reqLenStr))
        perror("write reqlen error");
    // 把换行符发送到服务端
    if (write(cfd, "\n", 1) != 1)
        perror("write newline error");

    if (-1 == (numRead = readLine(cfd, seqNumStr, INT_LEN)))
        perror("readLine error");
    if (0 == numRead)
        perror("unexpected EOF from server");
    // 从服务器读到的数据打印出来
    printf("sequence number: %s", seqNumStr);
    
    exit(EXIT_SUCCESS);
}
