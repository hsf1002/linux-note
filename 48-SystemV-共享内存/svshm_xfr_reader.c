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
#include "svshm_xfr.h"
#include "svsem_binary.h"


/**
 *   
 *将 一个System V的共享内存段中的数据传输到标准输出

read.c write.c   shmseg->shmget
svsem_binary.h 

LD_LIBRARY_PATH=. ./svshm_xfr_write
hello world, this is sky

^Z
[1]+  已停止               LD_LIBRARY_PATH=. ./svshm_xfr_write
LD_LIBRARY_PATH=. ./svshm_xfr_read 
hello world, this is sky


 */
int
main(int argc, char *argv[])    
{
    int sem_id, shm_id, bytes, xfrs;
    struct shmseg *shmp;

    // 获取包含两个信号量的信号量集
    if (-1 == (sem_id = semget(SEM_KEY, 0, 0)))
    {
        perror("semget get error");
        exit(EXIT_FAILURE);
    }

    // 获取共享内存
    if (-1 == (shm_id = shmseg(SHM_KEY, 0, 0)))
    {
        perror("shmseg get error");
        exit(EXIT_FAILURE);
    }

    // 将共享内存附加到系统选择的地址
    if ((void *)-1 == (shmp = shmat(shm_id, NULL, SHM_RDONLY)))
    {
        perror("shmat error");
        exit(EXIT_FAILURE);
    }

    for (xfrs=0, bytes=0; ; xfrs++)
    {
        // 减小读者的信号量
        if (-1 == reserve_sem(sem_id, READ_SEM))
        {
            perror("reserve_sem error");
        }

        // 读取完毕
        if (0 == shmp->cnt)
            break;
        
        bytes += shmp->cnt;

        // 从共享内存读取数据，并写入到标准输出
        if (shmp->cnt != write(STDOUT_FILENO, shmp->buf, shmp->cnt))
        {
            perror("write error");
        }

        // 增加写者的信号量
        if (-1 == release_sem(sem_id, WRITE_SEM))
        {
            perror("release_sem error");
        }
    }

    // 分离共享内存
    if (-1 == shmdt(shmp))
    {
        perror("shmdt error");
    }

    // 写者减小信号量
    if (-1 == release_sem(sem_id, WRITE_SEM))
    {
        perror("release_sem error");
    }
    
    fprintf(stderr, "received %d bytes (%d xfrs)\n", bytes, xfrs);
        
    exit(EXIT_SUCCESS);
}

