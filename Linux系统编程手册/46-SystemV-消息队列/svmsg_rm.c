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
 *   删除System V消息队列


 */
int
main(int argc, char *argv[])    
{
    if (argc > 1 && 0 == strcmp(argv[1], "--help"))
    {
        perror("wrong parameter number");
        exit(EXIT_FAILURE);
    }

    for (int i=1; i<argc; i++)
    {
        if (-1 == msgctl(getInt(argv[i], 0, "msgid"), IPC_RMID, NULL))
            fprintf(stderr, "msgctl delete %d error", i);
    }

    exit(EXIT_SUCCESS);
}

