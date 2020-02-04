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
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <stddef.h>
#include <signal.h>
#include "get_num.h"


/**
 *   System V 信号量，初始化一个信号量集
 * 

 */
int
main(int argc, char *argv[])    
{
    struct sembuf buf[2];
    int sem_id, key, perms;

    if (argc != 2 || 0 == strcmp(argv[1], "--help"))
    {
        fprintf(stderr, "%s [sem-op] \n", argv[0]);
        exit(EXIT_FAILURE);
    }

    key = 12345;
    perms = S_IRUSR | S_IWUSR;

    // 创建包含一个信号量的信号量集
    if (-1 != (sem_id = semget(key, 1, IPC_CREAT | IPC_EXCL | perms)))
    {
        union semun arg;
        struct sembuf sop;

        sleep(5);
        printf("%ld create semaphore\n", (long)getpid());

        arg.val = 0;
        // 初始化第一个信号量
        if (-1 == semctl(sem_id, 0, SETVAL, arg))
        {
            perror("semctl error");
            exit(EXIT_FAILURE);
        }

        sop.sem_num = 0; // 第一个信号量
        sop.sem_op = 0;  // 等待信号量值为0
        sop.sem_flg = 0; // 没有标记

        // 此处会更改otime的值，应该先走
        if (-1 == semop(sem_id, &sop, 1))
        {
            perror("semop error");
            exit(EXIT_FAILURE);
        }
        printf("%ld completed dummy semop()\n", (long)getpid());
    }
    else
    {
        // 创建失败的未知错误
        if (EEXIST != errno)
        {
            perror("semget create unexpected error");
            exit(EXIT_FAILURE);
        }
        // 获取已存在的信号量集
        else
        {
            const int MAX_TRIES = 10;
            union semun arg;
            struct semid_ds ds;

            if (-1 == (sem_id = semget(key, 1, perms)))
            {
                perror("semget get error");
                exit(EXIT_FAILURE);
            }

            printf("%ld got semaphore key\n", (long)getpid());

            arg.buf = &ds;

            // 尝试10次，根据otime判断，如果不为0，说明已经更改，否则等待
            for (int i=0; i<MAX_TRIES; i++)
            {
                printf("try %d\n", i);
                if (-1 == semctl(sem_id, 0, IPC_STAT, arg))
                    perror("setctrl IPC_STAT error");
                
                if (ds.sem_otime != 0)
                    break;
                sleep(1);
            }
            // 理论上走不进来，10s内创建+semop操作会完成
            if (ds.sem_otime == 0)
            {
                perror("existing semaphore not intitialized");
                exit(EXIT_FAILURE);
            }
        } 
    }
    
    buf[0].sem_num = 0;
    buf[0].sem_op = getInt(argv[1], 0, "sem-op");
    buf[0].sem_flg = 0;

    if (-1 == semop(sem_id, buf, 1))
    {
        perror("semop error");
        exit(EXIT_FAILURE);
    }
        
    exit(EXIT_SUCCESS);
}

