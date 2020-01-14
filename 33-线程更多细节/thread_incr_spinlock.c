#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <pthread.h>

static volatile glob = 0;
static pthread_spinlock_t splock;


/**
 * 
 */
static void *
thr_func(void *arg)
{
    int loops = *((int *)arg);
    int loc;
    int s;

    for (int i=0; i<loops; i++)
    {
        if (0 != (s = pthread_spin_lock(&splock)))
            perror("pthread_spin_lock error");
        
        loc = glob;
        loc++;
        glob = loc;

        if (0 != (s = pthread_spin_unlock(&splock)))
            perror("pthread_spin_unlock error");
    }

    return NULL;
}

/**
 *    
cc -g -Wall -o thread_incr_spinlock thread_incr_spinlock.c -lpthread

./thread_incr_spinlock 
main thread glob: 200000

 */
int
main(int argc, char *argv[])    
{
    pthread_t t1, t2;
    int s;
    int loops = 100000;

    if (0 != (s = pthread_spin_init(&splock, NULL)))
        perror("pthread_spin_init error");

    if (0 != (s = pthread_create(&t1, NULL, thr_func, &loops)))
        perror("pthread_create error");
    
    if (0 != (s = pthread_create(&t2, NULL, thr_func, &loops)))
        perror("pthread_create error");
    
    if (0 != (s = pthread_join(t1, NULL)))
        perror("pthread_join error");
    
    if (0 != (s = pthread_join(t2, NULL)))
        perror("pthread_join error");

    printf("main thread glob: %d\n", glob);

    exit(EXIT_SUCCESS);
}

