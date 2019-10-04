#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>


#define SV_SOCK_PATH "/tmp/us_xfr"
#define BUF_SIZE 100
#define BACKLOG   5

/**
 * UNIX domain流socket客户端
 */
int 
main(int argc, char **argv)
{
    struct sockaddr_un addr;
    int sfd;
    ssize_t numRead;
    char buf[BUF_SIZE];

    // 创建一个连接
    if (-1 == (sfd = socket(AF_UNIX, SOCK_STREAM, 0)))
        perror("socket error");
    
    memset(&addr, 0x00, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SV_SOCK_PATH, sizeof(addr.sun_path) - 1);
    // 连接到服务端
    if (-1 == connect(sfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)))
        perror("connect error");

    // 循环从标准输入读取数据
    while((numRead = read(STDIN_FILENO, buf, BUF_SIZE)) > 0)
        // 读取的数据写到服务端
        if (write(sfd, buf, numRead) != numRead)
            perror("write error");
    // 读取失败
    if (-1 == numRead)
        perror("read error");
    exit(EXIT_SUCCESS);
}
