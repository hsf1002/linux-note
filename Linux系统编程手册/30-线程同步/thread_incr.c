
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

/**
 * 
 */
static void *
thr_fun(void *arg)
{
    int loops = *((int *)arg);
    int loc;

    for (int i=0; i<loops; i++)
    {
        loc = glob;
        loc++;
        glob = loc;
    }

    return NULL;
}

/**
 *   
skydeiMac:30-线程同步 sky$ ./thread_incr 100
main glob = 200
skydeiMac:30-线程同步 sky$ ./thread_incr 1000
main glob = 2000
skydeiMac:30-线程同步 sky$ ./thread_incr 5000
main glob = 9294
skydeiMac:30-线程同步 sky$ ./thread_incr 6000
main glob = 6062
skydeiMac:30-线程同步 sky$ ./thread_incr 8000
main glob = 11528
skydeiMac:30-线程同步 sky$ ./thread_incr 10000
main glob = 10000
skydeiMac:30-线程同步 sky$ ./thread_incr 10000
main glob = 10478 
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

