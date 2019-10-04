#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>


#define SV_SOCK_PATH "/tmp/ud_case"
#define BUF_SIZE 10


/**
 * UNIX domain数据报socket服务端
 */
int 
main(int argc, char **argv)
{
    struct sockaddr_un svaddr, claddr;
    int sfd, j;
    socklen_t len;
    ssize_t numBytes;
    char buf[BUF_SIZE];

    // 创建一个连接
    if (-1 == (sfd = socket(AF_UNIX, SOCK_DGRAM, 0)))
        perror("socket error");
    // 如果路径存在，先删除，防止绑定失败
    if (-1 == remove(SV_SOCK_PATH) && ENOENT != errno)
        perror("remove error");

    memset(&svaddr, 0x00, sizeof(struct sockaddr_un));
    svaddr.sun_family = AF_UNIX;
    strncpy(svaddr.sun_path, SV_SOCK_PATH, sizeof(svaddr.sun_path) - 1);

    // 绑定到一个独一无二的地址
    if (-1 == bind(sfd, (struct sockaddr *)&svaddr, sizeof(struct sockaddr_un)))
        perror("bind error");

    // 循环发送，接收数据
    for(;;)
    {
        len = sizeof(struct sockaddr_un);

        // 从客户端接收数据
        if (-1 == (numBytes = (recvfrom(sfd, buf, BUF_SIZE, 0, (struct sockaddr *)&claddr, &len))))
            perror("recvfrom error");
        else
            printf("server: %ld bytes from %s\n", (int)numBytes, svaddr.sun_path);
        // 所有字符转为大写    
        for (int i=0; i<numBytes; i++)
            buf[i] = toupper((unsigned char )buf[i]);

        // 发送数据到客户端
        if (sendto(sfd, buf, numBytes, 0, (struct sockaddr *)&claddr, len) != numBytes)
            perror("sendto error");
        
    }
}
