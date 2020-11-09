#include <signal.h>
#include "fifo_seqnum.h"


/**
 *   使用FIFO的迭代式服务器
 */
int
main(int argc, char *argv[])    
{
    char client_fifo[CLIENT_FIFO_NAME_LEN];
    int server_fd;
    int client_fd;
    int dummy_fd;
    struct request req;
    struct response res;
    int seq_num = 0;
    
    umask(0);

    // 创建服务器端的FIFO
    if (-1 == mkfifo(SERVER_FIFO, S_IRUSR | S_IWUSR | S_IWGRP) && errno != EEXIST)
    {
        perror("mkfifo error");
        exit(EXIT_FAILURE);
    }

    // 只读方式打开服务器的FIFO
    if (-1 == (server_fd = open(SERVER_FIFO, O_RDONLY)))
    {
        perror("open error");
        exit(EXIT_FAILURE);
    }

    // 读写方式打开服务器的FIFO, 目的是永远不会出现EOF
    if (-1 == (dummy_fd = open(SERVER_FIFO, O_RDONLY)))
    {
        perror("open error");
        exit(EXIT_FAILURE);
    }

    // 忽略SIGPIPE信号
    if (SIG_ERR == signal(SIGPIPE, SIG_IGN))
    {
        perror("signal error");
        exit(EXIT_FAILURE);
    }

    for (;;)
    {
        // 读取客户端发送到服务器端FIFO的消息
        if (read(server_fd, &req, sizeof(struct request)) != sizeof(struct request))
        {
            fprintf(stderr, "error reading request, discarding \n");
            continue;
        }

        printf("server: client-pid = %ld\n", req.pid);

        // 组装客户端ID
        snprintf(client_fifo, CLIENT_FIFO_NAME_LEN, CLIENT_FIFO_TEMPLATE, (long)req.pid);

        if (-1 == (client_fd = open(client_fifo, O_WRONLY)))
        {
            fprintf(stderr, "open client error, continue\n");
            continue;
        }

        res.seq_num = seq_num;
        // 回复客户端
        if (write(client_fd, &res, sizeof(struct response)) != sizeof(struct response))
            perror("write to client error");

        if (-1 == close(client_fd))
            perror("close client fd error");
        
        seq_num = req.seq_len;
    }

    exit(EXIT_SUCCESS);
}

