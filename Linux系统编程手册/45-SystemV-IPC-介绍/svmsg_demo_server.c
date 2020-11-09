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


// 应该是已存在文件或该进程可创建的文件
#define KEY_FILE "/etc/services"



/**
 *   清理服务器中的IPC对象(消息队列)
 */
int
main(int argc, char *argv[])    
{
    int msgid;
    key_t key;
    const int MQ_PERMS = S_IRUSR | S_IWUSR | S_IWGRP;

    // 根据ftok生成key
    if (-1 == (key = ftok(KEY_FILE, 1)))
    {
        perror("ftok error");
        exit(EXIT_FAILURE);
    }

    // 根据key创建或获取IPC对象
    while (-1 == (msgid = msgget(key, IPC_CREAT | IPC_EXCL | MQ_PERMS)))
    {
        // 如果对象已经存在
        if (EEXIST == errno)
        {
            // 忽略权限获取对象
            if (-1 == (msgid = msgget(key, 0)))
            {
                perror("failed to retrieve the old value");
                exit(EXIT_FAILURE);
            }

            // 删除对象
            if (-1 == msgctl(msgid, IPC_RMID, NULL))
            {
                perror("msgctl delete error");
                exit(EXIT_FAILURE);
            }

            printf("removed old message queue(id=%d)\n", msgid);
        }
        else
        {
            perror("some other error");
            exit(EXIT_FAILURE);
        }
    }

    // 继续其他工作
    ///

    exit(EXIT_SUCCESS);
}

