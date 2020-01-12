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
static pthread_rwlock_t rwlock;


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
        if (0 != (s = pthread_rwlock_wrlock(&rwlock)))
            perror("pthread_rwlock_wrlock error");
        
        loc = glob;
        loc++;
        glob = loc;

        if (0 != (s = pthread_rwlock_unlock(&rwlock)))
            perror("pthread_rwlock_unlock error");
    }

    return NULL;
}

/**
 *    
./a.out 
main thread glob: 200000
* 
 */
int
main(int argc, char *argv[])    
{
    pthread_t t1, t2;
    int s;
    int loops = 100000;

    if (0 != (s = pthread_rwlock_init(&rwlock, NULL)))
        perror("pthread_rwlock_init error");

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

