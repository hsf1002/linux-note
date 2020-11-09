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
 *   System V 信号量的一个监控程序
 * 

 */
int
main(int argc, char *argv[])    
{
    struct semid_ds ds;
    union semun arg, dummy;
    int sem_id;

    if (argc != 2 || 0 == strcmp(argv[1], "--help"))
    {
        fprintf(stderr, "%s [semid]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // 命令行指定semid
    sem_id = getInt(argv[1], 0, "sem-id");
    printf("semid = %d\n", sem_id);
    arg.buf = &ds;

    // 获取信号量相关数据结构
    if (-1 == semctl(sem_id, 0, IPC_STAT, arg))
        perror("semctl error");
    
    printf("semaphore changed: %s", ctime(&ds.sem_ctime)); // create time
    printf("last semop():      %s", ctime(&ds.sem_otime)); // owner time

    if (NULL == (arg.array = calloc(ds.sem_nsems, sizeof(arg.array[0]))))
    {
        perror("calloc error");
        exit(EXIT_FAILURE);
    }

    // 获取整个信号量集，忽略第二个参数
    if (-1 == semctl(sem_id, 0, GETALL, arg))
    {
        perror("semctl error");
        exit(EXIT_FAILURE);
    }

    // 打印上一个执行semop的进程pid，当前等待该信号量值增长的进程数，当前等待带信号量变成0的信号数 
    printf("sem # value    SEMPID    SEMCNT    SEMZCNT\n");

    for (int i=0; i<ds.sem_nsems; i++)
    {
        printf("%3d    %5d    %5d    %5d\n", i, arg.array[i], semctl(sem_id, i, GETPID, dummy),
                        semctl(sem_id, i, GETNCNT, dummy),
                        semctl(sem_id, i, GETZCNT, dummy));
    }
    
    exit(EXIT_SUCCESS);
}

