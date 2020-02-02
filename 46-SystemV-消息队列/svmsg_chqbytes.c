#define _GNU_SOURCE
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

/**
 *   修改System V消息队列的msg_qbytes字段


 */
int
main(int argc, char *argv[])    
{
    struct msqid_ds ds;
    int msgid;

    if (argc != 3 || 0 == strcmp(argv[1], "--help"))
    {
        fprintf(stderr, "%s msqid max-bytes \n", argv[0]);
        exit(EXIT_FAILURE);
    }

    msgid = getInt(argv[1], 0, "msgid");

    // 获取消息队列的数据结构
    if (-1 == msgctl(msgid, IPC_STAT, &ds))
    {
        perror("msgctl IPC_STAT error");
        exit(EXIT_FAILURE);
    }

    ds.msg_qbytes = getInt(argv[2], 0, "max-bytes");

    // 修改消息队列的字段
    if (-1 == msgctl(msgid, IPC_SET, &ds))
    {
        perror("msgctl IPC_SET error");
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}

