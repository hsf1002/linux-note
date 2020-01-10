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


/**
 * 
 */
static void *
thr_func(void *arg)
{
    printf("thread running \n");

    for (int i=0; ; i++)
    {
        printf("loop %d\n", i);
        sleep(1);
    }
    
    return NULL;
}


/**
 *    
cc -g -Wall -o thread_cancel thread_cancel.c -lpthread

./thread_cancel 
thread running 
loop 0
loop 1
loop 2
thread canceled successfully

 */
int
main(int argc, char *argv[])    
{
    int s;
    void *ret;
    pthread_t t1;

    if (0 != (s = pthread_create(&t1, NULL, thr_func, (void *)1)))
        perror("pthread_create error");
    
    sleep(3);

    // 取消线程
    if (0 != (s = pthread_cancel(t1)))
        perror("pthread_cancel error");

    // 获取线程结束状态
    if (0 != (s = pthread_join(t1, &ret)))
            perror("pthread_join error");
    
    if (PTHREAD_CANCELED == ret)
        printf("thread canceled successfully\n");
    else 
        printf("thread canceled failed\n");

    exit(EXIT_SUCCESS);
}
