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
#include <ctype.h>
#include "svsem_binary.h"


bool bs_use_undo = false;
bool bs_retry = true;

/**
 * 信号量值更改1，可用状态
 */
int init_sem_available(int sem_id, int sem_num)
{
    union semun arg;

    arg.val = 1;
    return semctl(sem_id, sem_num, SETVAL, arg);
}

/**
 * 信号量值更改0，不可用状态
 */
int init_sem_inuse(int sem_id, int sem_num)
{
    union semun arg;

    arg.val = 0;
    return semctl(sem_id, sem_num, SETVAL, arg);
}

/**
 * 预留：将信号量的值减-1
 * 
 */
int reserve_sem(int sem_id, int sem_num)
{
    struct sembuf sops;

    sops.sem_num = sem_num;
    sops.sem_op = -1;
    sops.sem_flg = bs_use_undo ? SEM_UNDO : 0;

    while (-1 == semop(sem_id, &sops, 1))
        if (errno != EINPROGRESS || !bs_retry)
            return -1;
    
    return 0;
}

/**
 * 释放：将信号量的值加1
 * 
 */
int release_sem(int sem_id, int sem_num)
{
    struct sembuf sops;

    sops.sem_num = sem_num;
    sops.sem_op = 1;
    sops.sem_flg = bs_use_undo ? SEM_UNDO : 0;
    
    return semop(sem_id, &sops, 1);
}
