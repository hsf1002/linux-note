
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
cons_1: 9995
cons_2: 0
cons_1: 494
cons_2: 0
cons_1: 1120
cons_2: 0
cons_1: 115
cons_2: 0
cons_1: 211
cons_2: 0
cons_1: 21
cons_2: 0
cons_1: 529
cons_2: 0
cons_1: 124
cons_2: 0
cons_1: 401
cons_2: 0
cons_1: 966
cons_2: 0
cons_1: 2
cons_2: 0
cons_1: 983
cons_2: 0
cons_1: 262
cons_2: 0
cons_1: 206
cons_2: 0
...

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

    sleep(2);

    exit(EXIT_SUCCESS);
}

