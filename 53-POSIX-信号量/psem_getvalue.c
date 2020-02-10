//#define _BSD_SOURCE
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
#include <sys/mman.h>
#include <sys/file.h>
#include <stddef.h>
#include <signal.h>
#include <ctype.h>
#include <semaphore.h>


/**
 * 
 * 使用sem_getvalue获取POSIX命名信号量
创建一个信号量
./psem_create -c /demo 600 0
// 初始值为0，则阻塞
./psem_wait /demo &
[4] 46986
./psem_getvalue /demo
0

./psem_post /demo
./psem_post /demo
./psem_getvalue /demo
./psem_unlink /demo 
 * 
 */
int main(int argc, char *argv[])
{
    sem_t *sem;
    int value;
    
    if (argc != 2 || 0 == strcmp(argv[1], "--help"))
        fprintf(stderr, "% sem-name\n", argv[0]);
    
    // 0表示访问而非创建一个信号量
    if (SEM_FAILED == (sem = sem_open(argv[1], 0)))
    {
        perror("sem_open error");
        exit(EXIT_FAILURE);
    }

    // 如果有一个或多个进程正在阻塞等待信号量值递减，则Linux下value返回0
    if (-1 == (sem = sem_getvalue(sem, &value)))
    {
        perror("sem_getvalue error");
        exit(EXIT_FAILURE);
    }

    printf("%d\n", value);

    exit(EXIT_SUCCESS);
}

