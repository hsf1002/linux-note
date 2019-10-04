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
 * UNIX domain数据报socket客户端
 
 ./dgram_client hello world sky lotus chery
server: 5 bytes from /tmp/ud_case
client: 1: HELLO
server: 5 bytes from /tmp/ud_case
client: 2: WORLD
server: 3 bytes from /tmp/ud_case
client: 3: SKY
server: 5 bytes from /tmp/ud_case
client: 4: LOTUS
server: 5 bytes from /tmp/ud_case
client: 5: CHERY
 */
int 
main(int argc, char **argv)
{
    struct sockaddr_un svaddr, claddr;
    int sfd, j;
    size_t msgLen;
    ssize_t numBytes;
    char resp[BUF_SIZE];

    if (argc < 2 || strcmp(argv[1], "--help") == 0)
        fprintf(stderr, "%s msg... \n", argv[0]);

    // 创建一个连接
    if (-1 == (sfd = socket(AF_UNIX, SOCK_DGRAM, 0)))
        perror("socket error");
    
    memset(&claddr, 0x00, sizeof(struct sockaddr_un));
    claddr.sun_family = AF_UNIX;
    // 客户端地址的唯一性通过其进程ID保证
    snprintf(claddr.sun_path, sizeof(claddr.sun_path), "/tmp/ud_ucase_cl.%ld", (long)getpid());

    // 绑定到一个独一无二的地址，这样服务端就可以发送响应
    if (-1 == bind(sfd, (struct sockaddr *)&claddr, sizeof(struct sockaddr_un)))
        perror("bind error");

    memset(&svaddr, 0x00, sizeof(struct sockaddr_un));
    svaddr.sun_family = AF_UNIX;
    strncpy(svaddr.sun_path, SV_SOCK_PATH, sizeof(svaddr.sun_path) - 1);

    // 循环发送，接收数据
    for(j=1; j<argc; j++)
    {
        msgLen = strlen(argv[j]);

        // 发送数据到服务端
        if (sendto(sfd, argv[j], msgLen, 0, (struct sockaddr *)&svaddr, sizeof(struct sockaddr_un)) != msgLen)
            perror("sendto error");
        // 从服务端接收数据
        if (-1 == (numBytes = (recvfrom(sfd, resp, BUF_SIZE, 0, NULL, NULL))))
            perror("recvfrom error");
        else
            printf("client: %d: %.*s\n", j, (int)numBytes, resp);
        
    }
    remove(claddr.sun_path);
    exit(EXIT_SUCCESS);
}
