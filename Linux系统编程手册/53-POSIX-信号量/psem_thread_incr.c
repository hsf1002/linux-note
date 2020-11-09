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
#include <pthread.h>
#include "get_num.h"

static int glob = 0;
static sem_t sem;

static void *
thread_func(void *arg)
{
    int loops = *((int *)arg);
    int loc;

    for (int i=0; i<loops; i++)
    {
        if (-1 == sem_wait(&sem))
        {
            perror("sem_wait error");
            exit(EXIT_FAILURE);
        }

        loc = glob;
        loc++;
        glob = loc;

        if (-1 == sem_post(&sem))
        {
            perror("sem_post error");
            exit(EXIT_FAILURE);
        }
    }
}

/**
 * 
 * 使用POSIX未命名信号量  同步全局变量
 * 
 * 


 */
int main(int argc, char *argv[])
{
    pthread_t t1, t2;
    int loops, s;

    loops = (argc > 1) ? getInt(argv[1], GN_GT_0, "num-loops") : 10000;
    
    // 创建一个未命名信号量，0表示线程间共享，初始值为1
    if (SEM_FAILED == (sem = sem_init(&sem, 0, 1)))
    {
        perror("sem_open error");
        exit(EXIT_FAILURE);
    }

    if (0 != (s = pthread_create(&t1, NULL, thread_func, &loops)))
    {
        perror("pthread_create error");
        exit(EXIT_FAILURE);
    }

    if (0 != (s = pthread_create(&t2, NULL, thread_func, &loops)))
    {
        perror("pthread_create error");
        exit(EXIT_FAILURE);
    }

    if (0 != (s = pthread_join(t1, NULL)))
    {
        perror("pthread_join error");
        exit(EXIT_FAILURE);
    }

    if (0 != (s = pthread_join(t2, NULL)))
    {
        perror("pthread_join error");
        exit(EXIT_FAILURE);
    }

    printf("glob = %d\n", glob);

    exit(EXIT_SUCCESS);
}

