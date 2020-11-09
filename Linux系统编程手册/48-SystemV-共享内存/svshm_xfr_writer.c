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
 * 

 */
int
main(int argc, char *argv[])    
{
    int sem_id, shm_id, bytes, xfrs;
    struct shmseg *shmp;
    union semun dummy;

    // 创建包含两个信号量的信号量集
    if (-1 == (sem_id = semget(SEM_KEY, 2, IPC_CREAT | OBJ_PERMS)))
    {
        perror("semget create error");
        exit(EXIT_FAILURE);
    }

    // 写的进程必须先启动
    if (-1 == init_sem_available(sem_id, WRITE_SEM))
    {
        perror("init_sem_available error");
        exit(EXIT_FAILURE);
    }

    if (-1 == init_sem_inuse(sem_id, READ_SEM))
    {
        perror("init_sem_inuse error");
        exit(EXIT_FAILURE);
    }

    // 创建共享内存
    if (-1 == (shm_id = shmseg(SHM_KEY, sizeof(struct shmseg), IPC_CREAT | OBJ_PERMS)))
    {
        perror("shmseg create error");
        exit(EXIT_FAILURE);
    }

    // 将共享内存附加到系统选择的地址
    if ((void *)-1 == (shmp = shmat(shm_id, NULL, 0)))
    {
        perror("shmat error");
        exit(EXIT_FAILURE);
    }

    for (xfrs=0, bytes=0; ; xfrs++, bytes += shmp->cnt)
    {
        // 减小写者的信号量
        if (-1 == reserve_sem(sem_id, WRITE_SEM))
        {
            perror("reserve_sem error");
        }

        // 从标准输入读取数据，并写入到共享内存
        if (-1 == (shmp->cnt = read(STDIN_FILENO, shmp->buf, BUF_SIZE)))
        {
            perror("read error");
        }

        // 增加读者的信号量
        if (-1 == release_sem(sem_id, READ_SEM))
        {
            perror("release_sem error");
        }

        // 读取完毕
        if (0 == shmp->cnt)
            break;
    }

    // 写者再次减小信号量，这样就能知道读者已经完成了对共享内存的最后一次访问了
    if (-1 == reserve_sem(sem_id, WRITE_SEM))
    {
        perror("reserve_sem error");
    }

    // 删除信号量集
    if (-1 == semctl(sem_id, 0, IPC_RMID, dummy))
    {
        perror("semctl IPC_RMID error");
    }

    // 分离共享内存
    if (-1 == shmdt(shmp))
    {
        perror("shmdt error");
    }

    // 删除共享内存
    if (-1 == shmctl(shm_id, IPC_RMID, 0))
    {
        perror("shmctl IPC_RMID error");
    }
    
    fprintf(stderr, "sent %d bytes (%d xfrs)\n", bytes, xfrs);
        
    exit(EXIT_SUCCESS);
}

