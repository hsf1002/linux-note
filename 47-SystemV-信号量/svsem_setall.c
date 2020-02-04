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
 *   System V 信号量，使用SETALL操作初始化一个信号量集
 * 

 */
int
main(int argc, char *argv[])    
{
    struct semid_ds ds;
    union semun arg, dummy;
    int sem_id;

    if (argc < 3 || 0 == strcmp(argv[1], "--help"))
    {
        fprintf(stderr, "%s [semid] val... \n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // 命令行指定semid
    sem_id = getInt(argv[1], 0, "sem-id");

    arg.buf = &ds;
    printf("semid = %d\n", sem_id);

    // 获取信号量相关数据结构
    if (-1 == semctl(sem_id, 0, IPC_STAT, arg))
        perror("semctl error");
    
    // 确保提供的参数个数与信号量个数相同
    if (argc - 2 !=  ds.sem_nsems)
    {
        fprintf(stderr, "%s set %d semaphores, but %d parmas supplied\n", argv[0], (long)ds.sem_nsems, argc - 2);
        exit(EXIT_FAILURE);
    }
    
    if (NULL == (arg.array = calloc(ds.sem_nsems, sizeof(arg.array[0]))))
    {
        perror("calloc error");
        exit(EXIT_FAILURE);
    }

    for (int i=2; i<argc; i++)
    {
        arg.array[i - 2] = getInt(argv[i], 0, "val");
    }

    // 一次性设置信号量集的值
    if (-1 == semctl(sem_id, 0, SETALL, arg))
    {
        perror("semctl error");
        exit(EXIT_FAILURE);
    }

    printf("semaphore values changed (PID=%ld)\n", (long)getpid());
    
    exit(EXIT_SUCCESS);
}

