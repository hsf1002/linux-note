#include <signal.h>
#include "fifo_seqnum.h"
#include "get_num.h"

static char client_fifo[CLIENT_FIFO_NAME_LEN];

static void
remove_fifo(void)
{
    unlink(client_fifo);
}

/**
 *   使用FIFO的客户端
 */
int
main(int argc, char *argv[])    
{
    int server_fd;
    int client_fd;
    struct request req;
    struct response res;
    
    umask(0);

    snprintf(client_fifo, CLIENT_FIFO_NAME_LEN, CLIENT_FIFO_TEMPLATE, (long)getpid());

    // 创建客户端的FIFO
    if (-1 == mkfifo(client_fifo, S_IRUSR | S_IWUSR | S_IWGRP) && errno != EEXIST)
    {
        perror("mkfifo error");
        exit(EXIT_FAILURE);
    }

    // 注册注销函数
    if (0 != atexit(remove_fifo))
        perror("atexit error");
    
    req.pid = getpid();
    req.seq_len = (argc > 1) ? getInt(argv[1], GN_GT_0, "seq-len") : 1;

    // 只写方式打开服务器的FIFO
    if (-1 == (server_fd = open(SERVER_FIFO, O_WRONLY)))
    {
        perror("open error");
        exit(EXIT_FAILURE);
    }

    // 向服务器端发送消息
    if (write(server_fd, &req, sizeof(struct request)) != sizeof(struct request))
        perror("write to server error");
        
    // 只读方式打开客户端管道
    if (-1 == (client_fd = open(client_fifo, O_RDONLY)))
    {
        fprintf(stderr, "open client error\n");
        exit(EXIT_FAILURE);
    }

    // 读取服务器发送到客户端FIFO的消息
    if (read(client_fd, &res, sizeof(struct response)) != sizeof(struct response))
    {
        fprintf(stderr, "error reading response \n");
        exit(EXIT_FAILURE);
    }
    
    // 打印读取到的内容的字节数
    printf("%d\n", res.seq_num);

    exit(EXIT_SUCCESS);
}

