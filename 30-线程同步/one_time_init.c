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


struct once_init
{
    pthread_mutex_t mtx;
    int called;
};

#define ONCE_INITIALIZER {PTHREAD_MUTEX_INITIALIZER, 0} 

struct once_init once = ONCE_INITIALIZER;


/**
 * 
 */
static int
one_time_init(struct once_init *one_control, void (*init)(void)) 
{
    int s;

    printf("one_time_init \n");

    if (0 != (s = pthread_mutex_lock(&(one_control->mtx))))
        perror("pthread_mutex_lock error");

    // 在此控制init的调用
    if (!one_control->called)
    {
        (*init)();
        one_control->called = 1;
    }
    
    if (0 != (s = pthread_mutex_unlock(&(one_control->mtx))))
        perror("pthread_mutex_unlock error");
    
    return 0;
}

/**
 * 
*/
static void 
init_func()
{
    printf("called init_func()\n");
}

/**
 * 
 */
static void *
thr_func(void *arg)
{
    printf("thread %ld running \n", (int)arg);

    // 每个线程都调用三次one_time_init，两个线程共调用六次one_time_init，但是init_func只调用了一次
    one_time_init(&once, init_func);
    one_time_init(&once, init_func);
    one_time_init(&once, init_func);
    
    return NULL;
}


/**
 *    
cc -g -Wall -o one_time_init one_time_init.c -lpthread

./one_time_init 
thread 1 running 
one_time_init 
called init_func()
one_time_init 
one_time_init 
thread 2 running 
one_time_init 
one_time_init 
one_time_init 
first thread returned
second thread returned
 */
int
main(int argc, char *argv[])    
{
    int s;
    pthread_t t1, t2;

    if (0 != (s = pthread_create(&t1, NULL, thr_func, (void *)1)))
        perror("pthread_create error");
    
    if (0 != (s = pthread_create(&t2, NULL, thr_func, (void *)2)))
        perror("pthread_create error");

    if (0 != (s = pthread_join(t1, NULL)))
        perror("pthread_join error");

    printf("first thread returned\n");

    if (0 != (s = pthread_join(t2, NULL)))
            perror("pthread_join error");

    printf("second thread returned\n");

    exit(EXIT_SUCCESS);
}
