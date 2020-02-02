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


/**
 *   显示系统上所有的System V消息队列
mscOS未定义MSG_INFO

 */
int
main(int argc, char *argv[])    
{
    struct msqid_ds ds;
    struct msginfo info;
    int msgid, maxind;

    if (-1 == (maxind = msgctl(0, MSG_INFO, (struct msgid_ds *)&info)))
    {
        perror("msgctl MSG_INFO error");
        exit(EXIT_FAILURE);
    }

    printf("maxind: %d\n\n", msgid);
    printf("index    id    key    messages\n");

    for(int i=0; i<=maxind; i++)
    {
        if (-1 == (msgid = msgctl(i, MSG_STAT, &ds)))
        {
            if (errno != EINVAL && errno != EACCES)
                perror("msgctl unexpected error");
            continue;
        }

        printf("%4d %8d 0x%08lx %7ld\n", i, msgid, (unsigned long)ds.msg_perm._key, (long)ds.msg_qnum);
    }

    exit(EXIT_SUCCESS);
}

