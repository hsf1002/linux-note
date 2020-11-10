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
 * 使用sem_post递增一个POSIX命名信号量
 * 
 */
int main(int argc, char *argv[])
{
    sem_t *sem;
    
    if (argc != 2 || 0 == strcmp(argv[1], "--help"))
        fprintf(stderr, "% sem-name\n", argv[0]);
    
    // 0表示访问而非创建一个信号量
    if (SEM_FAILED == (sem = sem_open(argv[1], 0)))
    {
        perror("sem_open error");
        exit(EXIT_FAILURE);
    }

    // 如果sem当前是0，有因此等待的进程或线程在调用之后将会唤醒
    if (-1 == (sem = sem_post(sem)))
    {
        perror("sem_post error");
        exit(EXIT_FAILURE);
    }

    printf("%ld sem_post() succeeded\n", (long)getpid());

    exit(EXIT_SUCCESS);
}

