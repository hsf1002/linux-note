
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <pthread.h>

static int avail = 0;
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

/**
 *  生产者线程
 */
static void *
thr_prod_fun(void *arg)
{
    int s;

    for (;;)
    {
        if (0 != (s = pthread_mutex_lock(&mtx)))
            perror("mutex lock error");

        //printf("prod_1: %d\n", avail);
        avail++;
        //printf("prod_2: %d\n", avail);

        if (0 != (s = pthread_mutex_unlock(&mtx)))
            perror("mutex lock error");
        
        // 可以和解锁顺序颠倒，在某些实现上可以获得更高性能
        if (0 != (s = pthread_cond_signal(&cond)))
            perror("mutex lock error");
    }
    return NULL;
}

/**
 *  消费者线程
 */
static void *
thr_cons_fun(void *arg)
{
    int s;
    bool flag = false;

    for (;;)
    {
        if (0 != (s = pthread_mutex_lock(&mtx)))
            perror("mutex lock error");

        if (avail > 1)
        {
            printf("cons_1: %d\n", avail);
            flag = true;
        }

        // 进入睡眠，等待唤醒
        while (avail == 0)
        {
            if (0 != (s = pthread_cond_wait(&cond, &mtx)))
                perror("cond wait error");
        }

        //printf("cons_1: %d\n", avail);
        while (avail > 0)
            avail--;
        //printf("cons_2: %d\n", avail);

        if (flag)
        {
            flag = false;
            printf("cons_2: %d\n", avail);
        }

        if (0 != (s = pthread_mutex_unlock(&mtx)))
            perror("mutex lock error");
    }
    return NULL;
}

/**
 *   
./a.out 
cons_1: 59
cons_2: 0
cons_1: 63
cons_2: 0
cons_1: 728
cons_2: 0
cons_1: 53
cons_2: 0
cons_1: 269
cons_2: 0
cons_1: 593
cons_2: 0
cons_1: 120
cons_2: 0
cons_1: 175
cons_2: 0
cons_1: 10
cons_2: 0
cons_1: 1124
cons_2: 0
cons_1: 5749
cons_2: 0
cons_1: 631
cons_2: 0
cons_1: 184
cons_2: 0
cons_1: 236
cons_2: 0
cons_1: 20
cons_2: 0
cons_1: 70
cons_2: 0
cons_1: 36
cons_2: 0
cons_1: 1261
cons_2: 0
* 
 */
int
main(int argc, char *argv[])    
{
    pthread_t t1, t2;
    int s;

    if (0 != (s = pthread_create(&t1, NULL, thr_prod_fun, NULL)))
        perror("pthread_create t1 error");

    if (0 != (s = pthread_create(&t2, NULL, thr_cons_fun, NULL)))
        perror("pthread_create t2 error");

    sleep(5);

    exit(EXIT_SUCCESS);
}

