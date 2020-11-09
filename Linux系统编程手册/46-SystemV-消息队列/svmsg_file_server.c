
#include "svmsg_file.h"


static void
grim_reaper(int sig)
{
    int saved_errno;

    // waitpid 可能修改errno
    saved_errno = errno;
    // 避免僵尸进程
    while (waitpid(-1, NULL, WNOHANG) > 0)
    {
        continue;
    }
    errno = saved_errno;
}

/**
 * 服务器返回给客户端的消息
 */
static void
serve_request(const struct request_msg *req)
{
    int fd;
    ssize_t num_read;
    struct response_msg res;

    // 如果打开请求的文件，发送一条失败的消息
    if (-1 == (fd = open(req->pathname, O_RDONLY)))
    {
        res.mtype = RES_MT_FAILURE;
        snprintf(res.data, sizeof(res.data), "%s", "could not open");
        msgsnd(req->client_id, &res, strlen(res.data) + 1, 0);
        exit(EXIT_FAILURE);
    }

    // 先发送数据消息
    res.mtype = RES_MT_DATA;
    while ((num_read = read(fd, res.data, RES_MSG_SIZE)) > 0)
        if (-1 == msgsnd(req->client_id, &res, num_read, 0))
            break;
    
    // 再发送一条长度为0的成功消息
    res.mtype = RES_MT_END;
    msgsnd(req->client_id, &res, 0, 0);
}

/**
 *   System V消息队列，服务器端

 */
int
main(int argc, char *argv[])    
{
    struct request_msg req;
    pid_t pid;
    ssize_t msg_len;
    int server_id;
    struct sigaction sa;

printf("server queue create\n");
    sleep(2);
    // 创建服务器的消息队列
    if (-1 == (server_id = msgget(SERVER_KEY, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR | S_IWGRP)))
    {
        perror("msgget create error");
        exit(EXIT_FAILURE);
    }
printf("server queue create ok\n");
    // 为SIGCHLD注册信号处理函数
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sa.sa_handler = grim_reaper;

    if (-1 == sigaction(SIGCHLD, &sa, NULL))
    {
        perror("sigaction error");
        exit(EXIT_FAILURE);
    }
printf("server prepare to receive msg\n");
    for (;;)
    {
        printf("111\n");
        // 接收客户端的消息
        if (-1 == (msg_len = msgrcv(server_id, &req, REQ_MSG_SIZE, 0, 0)))
        {
            // 被信号SIGCHLD中断，重启调用msgrcv
            if (EINTR == errno)
                continue;
            // 其他错误
            perror("msgrcv some other error");
            break;
        }
        else
        {
            printf("server msg_len = %d\n", msg_len);
        }

        // 对于每个接收的客户端的消息，创建子进程，单独去回复
        if (-1 == (pid = fork()))
        {
            perror("fork error");
            break;
        }

        if (0 == pid)
        {
            serve_request(&req);
            _exit(EXIT_SUCCESS);
        }
    }

    // 删除消息队列
    if (-1 == msgctl(server_id, IPC_RMID, NULL))
    {
        perror("msgctl IPC_RMID error");
        exit(EXIT_FAILURE);
    }
    
    exit(EXIT_SUCCESS);
}

