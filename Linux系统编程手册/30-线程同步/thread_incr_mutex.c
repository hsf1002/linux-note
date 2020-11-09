
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
#include "get_num.h"

static int glob = 0;
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

/**
 * 
 */
static void *
thr_fun(void *arg)
{
    int loops = *((int *)arg);
    int loc;
    int s;

    for (int i=0; i<loops; i++)
    {
        if (0 != (s = pthread_mutex_lock(&mtx)))
            perror("mutex lock error");

        loc = glob;
        loc++;
        glob = loc;

        if (0 != (s = pthread_mutex_unlock(&mtx)))
            perror("mutex lock error");
    }

    return NULL;
}

/**
 *   
skydeiMac:30-线程同步 sky$ ./thread_incr_mutex 100
main glob = 200
skydeiMac:30-线程同步 sky$ ./thread_incr_mutex 500
main glob = 1000
skydeiMac:30-线程同步 sky$ ./thread_incr_mutex 1000
main glob = 2000
skydeiMac:30-线程同步 sky$ ./thread_incr_mutex 5000
main glob = 10000
skydeiMac:30-线程同步 sky$ ./thread_incr_mutex 10000
main glob = 20000
skydeiMac:30-线程同步 sky$ ./thread_incr_mutex 50000
main glob = 100000
skydeiMac:30-线程同步 sky$ ./thread_incr_mutex 100000
main glob = 200000
skydeiMac:30-线程同步 sky$ ./thread_incr_mutex 500000
main glob = 1000000
skydeiMac:30-线程同步 sky$ ./thread_incr_mutex 1000000
main glob = 2000000
* 
 */
int
main(int argc, char *argv[])    
{
    pthread_t t1, t2;
    int loops;
    int s;

    loops = (argc > 1) ? getInt(argv[1], GN_GT_0, "num-loops") : 1000000;

    if (0 != (s = pthread_create(&t1, NULL, thr_fun, &loops)))
        perror("pthread_create t1 error");

    if (0 != (s = pthread_create(&t2, NULL, thr_fun, &loops)))
        perror("pthread_create t2 error");
    
    if (0 != (s = pthread_join(t1, NULL)))
        perror("pthread_join t1 error");

    if (0 != (s = pthread_join(t2, NULL)))
        perror("pthread_join t2 error");

    printf("main glob = %d\n", glob);

    exit(EXIT_SUCCESS);
}

