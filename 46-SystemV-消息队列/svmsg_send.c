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
#include <sys/ipc.h>
#include <sys/msg.h>
#include "get_num.h"


#define MAX_MTEXT 1024

struct mbuf
{
    long mtype;
    char mtext[MAX_MTEXT];
};


static void             
usage_error(const char *progName, const char *msg)
{
    if (msg != NULL)
        fprintf(stderr, "%s", msg);
    fprintf(stderr, "Usage: %s [-n] msqid msg-type [msg-text]\n", progName);
    fprintf(stderr, "    -n       Use IPC_NOWAIT flag\n");

    exit(EXIT_FAILURE);
}


/**
 *   使用System V消息队列，发送消息
 */
int
main(int argc, char *argv[])    
{
    int flag = 0, msgid = 0, opt = 0;
    int msg_len;
    struct mbuf msg;

    while (-1 != (opt = getopt(argc, argv, "n")))
    {
        if (opt == 'n')
            flag |= IPC_NOWAIT;
        else
            usage_error(argv[0], NULL);
    }

    if (argc < optind + 2 || argc > optind + 3)
        usage_error(argv[0], "wrong number of arguments\n");

    // 命令行指定msgid和消息类型    
    msgid = getInt(argv[optind], 0, "msgid");
    msg.mtype = getInt(argv[optind + 1], 0, "msg-type");

    // 提供消息内容
    if (argc > optind + 2)
    {
        if ((msg_len = strlen(argv[optind + 2]) + 1) > MAX_MTEXT)
        {
            perror("msg-text too long\n");
            exit(EXIT_FAILURE);
        }

        memcpy(msg.mtext, argv[optind + 2], msg_len);
    }
    else
    {
        msg_len = 0;
    }

    // 发送消息
    if (-1 == msgsnd(msgid, &msg, msg_len, flag))
    {
        perror("msgsnd error");
        exit(EXIT_FAILURE);
    }

    printf("svmsg send success, msgid = %d, msg_len = %d, msg_text = %s\n", msgid, msg_len, msg.mtext);

    exit(EXIT_SUCCESS);
}

