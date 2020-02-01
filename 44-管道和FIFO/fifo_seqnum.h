
//#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>

// 客户端服务器都知晓的服务器FIFO名称
#define SERVER_FIFO            "/tmp/seqnum_sv"
// 客户端FIFO名称模板
#define CLIENT_FIFO_TEMPLATE   "/tmp/seqnum_cl.%ld"
#define CLIENT_FIFO_NAME_LEN   (sizeof(CLIENT_FIFO_TEMPLATE) + 20)

// client->server
struct request
{
    pid_t pid;
    int seq_len;
};

// server->client
struct response
{
    int seq_num;
};

