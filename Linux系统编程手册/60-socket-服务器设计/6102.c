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


#define SERVICE "echo"
#define BUFSIZE 500



/**
 * 并发型的TCP echo客户端

需要有root权限
sh-3.2# ./tcp_echo_server
sh-3.2# ./tcp_echo_client localhost
1
1

hello
hello

world
world

beatifully sky lotus
beatifully sky lotus
*/
int 
main(int argc, char **argv)
{
    int sfd;
    ssize_t numRead;
    char buf[BUFSIZ];

    if (argc != 2 || strcmp(argv[1], "--help") == 0)
        fprintf(stderr, "%s: host\n", argv[0]);

    // 创建
    if (-1 == (sfd = inetConnect(argv[1], SERVICE, SOCK_STREAM)))
    {
        perror("connect socket error");
        exit(EXIT_FAILURE);
    }

    switch (fork())
    {
        case -1:
            perror("fork error");
            exit(EXIT_FAILURE);
        //break;
        // 子进程
        case 0:
        {
            for(;;)
            {
                // 从服务端读取数据打印
                if (-1 == (numRead = read(sfd, buf, BUFSIZ)))
                    perror("read error"); 
                printf("%.*s\n", (int)numRead, buf);
            }
            exit(EXIT_SUCCESS);
        }
        //break;
        // 父进程
        default:
        {
            for (;;)
            {
                // 从标准输入读取数据
                if (0 >= (numRead = read(STDIN_FILENO, buf, BUFSIZ)))
                    perror("read error"); 
                // 将数据写到服务端
                if (numRead != write(sfd, buf, numRead))
                    perror("write error");
            }

            // 检测到文件尾，关闭套接字上的写端，这将导致echo服务器检测到文件尾，它就会关闭套接字，进而导致客户端检测到文件尾
            if (shutdown(sfd, SHUT_WR) == -1)
                perror("shutdown wr error");
            exit(EXIT_SUCCESS);
        }
        //break;
    }

    exit(EXIT_SUCCESS);
}
