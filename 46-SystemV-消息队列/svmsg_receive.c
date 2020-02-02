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
    fprintf(stderr, "Usage: %s [options] msqid [max-bytes]\n", progName);
    fprintf(stderr, "Permitted options are:\n");
    fprintf(stderr, "    -e       Use MSG_NOERROR flag\n");
    fprintf(stderr, "    -t type  Select message of given type\n");
    fprintf(stderr, "    -n       Use IPC_NOWAIT flag\n");
#ifdef MSG_EXCEPT
    fprintf(stderr, "    -x       Use MSG_EXCEPT flag\n");
#endif
    exit(EXIT_FAILURE);
}


/**
 *   使用System V消息队列，接收消息
 */
int
main(int argc, char *argv[])    
{
    int flag = 0, msgid = 0, opt = 0, type = 0;
    ssize_t msg_len;
    struct mbuf msg;
    size_t max_byptes;

    while (-1 != (opt = getopt(argc, argv, "ent:x")))
    {
        switch (opt)
        {
            case 'e':
            {
                flag |= MSG_NOERROR;
            }
            break;
            case 'n':
            {
                flag |= IPC_NOWAIT;
            }
            break;
            case 't':
            {
                type = atoi(optarg);
            }
            break;
        #ifdef MSG_EXCEPT    
            case 'x':
            {
                flag | MSG_EXCEPT;
            }
            break;
        #endif
            default:
            {
                usage_error(argv[0], NULL);
            }
            break;
        }
    }

    if (argc < optind + 1 || argc > optind + 2)
        usage_error(argv[0], "wrong number of arguments\n");

    // 命令行指定msgid和消息长度   
    msgid = getInt(argv[optind], 0, "msgid");
    max_byptes = (argc > optind + 1) ? getInt(argv[optind + 1], 0, "max-bytes") : MAX_MTEXT;

    // 接收消息并打印到终端
    if (-1 == msgrcv(msgid, &msg, max_byptes, type, flag))
    {
        perror("msgrcv error");
        exit(EXIT_FAILURE);
    }

    printf("svmsg receive success, msgid = %d, msg_len = %d, mtype = %d\n", msgid, msg_len, msg.mtype);

    if (msg_len > 0)
        printf("svmsg msg: %s\n", msg.mtext);

    exit(EXIT_SUCCESS);
}

