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
#include <sys/sem.h>
#include <stddef.h>
#include <signal.h>
#include "get_num.h"


/**
 *   System V 创建和操作信号量

cc -g -Wall -o svsem_demo svsem_demo.c libgetnum.so

./svsem_demo 0
semaphore ID=327681
./svsem_demo 327681 -2 &
[1] 38854
./svsem_demo 327681 +3
38855: about to setop.
38855: semop completed.

[1]+  Stopped                 ./svsem_demo 327681 -2 
 */
int
main(int argc, char *argv[])    
{
    int sem_id;

    if (argc < 2 || argc > 3 || 0 == strcmp(argv[1], "--help"))
    {
        fprintf(stderr, "%s [init-value]/[option]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // 创建信号量集
    if (argc == 2)
    {
        union semun arg;
        // 创建包含一个信号量的信号量集
        if (-1 == (sem_id = semget(IPC_PRIVATE, 1, S_IRUSR | S_IWUSR)))
        {
            perror("semget create error");
            exit(EXIT_FAILURE);
        }

        arg.val = getInt(argv[1], 0, "init-value");

        // 给第一个信号量，设置初始值
        if (-1 == semctl(sem_id, 0, SETVAL, arg))
        {
            perror("semctl error");
            exit(EXIT_FAILURE);
        }

        printf("semaphore ID=%ld\n", sem_id);
    }
    else
    {
        struct sembuf buf;

        sem_id = getInt(argv[1], 0, "sem_id");

        // 表示操作第一个信号量
        buf.sem_num = 0;
        // 施加的操作
        buf.sem_op = getInt(argv[2], 0, "operation");
        buf.sem_flg = 0;

        printf("%ld: about to setop.\n", (long)getpid());

        if (-1 == semop(sem_id, &buf, 1))
        {
            perror("semop error");
            exit(EXIT_FAILURE);
        }

        printf("%ld: semop completed.\n", (long)getpid());
    }
    
    exit(EXIT_SUCCESS);
}

