
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
#include <sys/msg.h>
#include <stddef.h>
#include <signal.h>

#define SERVER_KEY  0x1aaaaaa1

// client-- >server
struct request_msg
{
    long mtype;     // unused
    int client_id;
    char pathname[PATH_MAX];
};

#define REQ_MSG_SIZE (offsetof(struct request_msg, pathname) - offsetof(struct request_msg, client_id) + PATH_MAX)

#define RES_MSG_SIZE 8192

struct response_msg
{
    long mtype;
    char data[RES_MSG_SIZE];
};

#define RES_MT_FAILURE 1
#define RES_MT_DATA    2
#define RES_MT_END     3
