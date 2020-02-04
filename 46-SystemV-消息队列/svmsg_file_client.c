#include "svmsg_file.h"


static int client_id;

static void
remove_queue()
{
    if (-1 == msgctl(client_id, IPC_RMID, NULL))
        perror("msgctl error");
}


/**
 *   System V消息队列，服务器端

 */
int
main(int argc, char *argv[])    
{
    struct request_msg req;
    struct response_msg res;
    ssize_t msg_len;
    ssize_t total_bytes;
    int num_msg;
    int server_id;

    if (argc != 2 || 0 == strcmp(argv[1], "--help"))
    {
        fprintf(stderr, "%s pathname\n", argv[0]);
        exit(EXIT_FAILURE);
    }
printf("stren = %d, sizeof = %d\n", strlen(argv[1]), sizeof(req.pathname));
    if (strlen(argv[1]) > sizeof(req.pathname) - 1)
    {
        perror("pathname too long");
        exit(EXIT_FAILURE);
    }

    // 获取服务器的消息队列
    if (-1 == (server_id = msgget(SERVER_KEY, S_IWUSR)))
    {
        perror("msgget get error");
        exit(EXIT_FAILURE);
    }

printf("server queue get ok\n");
    // 创建客户端的消息队列
    if (-1 == (client_id = msgget(IPC_PRIVATE, S_IRUSR | S_IWUSR | S_IWGRP)))
    {
        perror("msgget create error");
        exit(EXIT_FAILURE);
    }
printf("client queue create ok\n");
    // 注册注销函数以删除消息队列
    if (0 != atexit(remove_queue))
        perror("atexit error");
printf("atexit ok\n");    
    req.mtype = 1;
    req.client_id = client_id;
    strncpy(req.pathname, argv[1], sizeof(req.pathname) - 1);
    req.pathname[sizeof(req.pathname) - 1] = '\0';
printf("prepare to send msg\n");
    // 向服务器发送消息，传递客户端消息队列标识符和文件路径
    if (-1 == msgsnd(server_id, &req, REQ_MSG_SIZE, 0))
    {
        perror("msgsnd clientid error");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("client:send the first message\n");
    }
    // 接收服务器发送的第一条消息
    if (-1 == (msg_len = msgrcv(client_id, &res, RES_MSG_SIZE, 0, 0)))
        perror("msgrcv the first msg error");
    else
        printf("client:receive the first message, type=%d\n", res.mtype);
    // 原因是文件打开失败
    if (res.mtype == RES_MT_FAILURE)
    {
        printf("%s \n", res.data);
        exit(EXIT_FAILURE);
    }

    total_bytes = msg_len;
    printf("client:total = %ld\n", total_bytes);

    for (num_msg=1; res.mtype==RES_MT_DATA; num_msg++)
    {
        // 循环不断接收服务端的消息
        if (-1 == (msg_len = msgrcv(client_id, &res, RES_MSG_SIZE, 0, 0)))
        {
            perror("msgrcv data error");
            exit(EXIT_FAILURE);
        }

        total_bytes += msg_len;
    }

    printf("client received %ld bytes (%d messages)\n", (long)total_bytes, num_msg);
    
    exit(EXIT_SUCCESS);
}

