#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>


#define SV_SOCK_PATH "/tmp/us_xfr"
#define BUF_SIZE  100
#define BACKLOG   5

/**
 * UNIX domain流socket服务端
 *
    编译步骤：
     gcc -E 5703.c -o stream_server.i
     gcc -S stream_server.i -o stream_server.s
     gcc -c stream_server.s -o stream_server.o
     gcc stream_server.o -o stream_server

    执行步骤：
     ./stream_server > output.txt &
     ./stream_client < input.txt
     kill %1
     diff input.txt output.txt     
*/
int 
main(int argc, char **argv)
{
    struct sockaddr_un addr;
    int sfd, cfd;
    ssize_t numRead;
    char buf[BUF_SIZE];

    // 创建一个连接
    if (-1 == (sfd = socket(AF_UNIX, SOCK_STREAM, 0)))
        perror("socket error");
    // 如果路径存在，先删除，防止绑定失败
    if (-1 == remove(SV_SOCK_PATH) && ENOENT != errno)
        perror("remove error");
    
    memset(&addr, 0x00, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SV_SOCK_PATH, sizeof(addr.sun_path) - 1);
    // 绑定到一个众所周知的绝对路径
    if (-1 == bind(sfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)))
        perror("bind error");
    // 宣告愿意接受连接请求，且未决的连接数量为5
    if (-1 == listen(sfd, BACKLOG))
        perror("listen error");

    // 循环迭代处理客户端连接请求
    for (;;)
    {
        // 接受一个连接，并返回一个新socket
        if (-1 == (cfd = accept(sfd, NULL, NULL)))
            perror("accept error");
        // 循环从客户端读取数据
        while((numRead = read(cfd, buf, BUF_SIZE)) > 0)
            // 读取的数据写到标准输出
            if (write(STDOUT_FILENO, buf, numRead) != numRead)
                perror("write error");
        // 读取失败
        if (-1 == numRead)
            perror("read error");
        // 关闭客户端失败
        if (-1 == close(cfd))
            perror("close error");
    }
}
