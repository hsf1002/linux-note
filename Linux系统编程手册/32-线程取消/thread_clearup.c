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

static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static int glob = 0;


/**
 *清理函数 
 */
static void 
clearup_handler(void *arg)
{
    int s;

    printf("clearup: freeing block at %p \n", arg);
    free(arg);

    printf("clearup: unlocking mutex\n");

    if (0 != (s = pthread_mutex_unlock(&mtx)))
        perror("pthread_mutex_unlock error");
}

/**
 * 
 */
static void *
thr_func(void *arg)
{
    int s;
    void *buf = NULL;

    // malloc不是取消点
    if (NULL == (buf = malloc(0x10000)))
        perror("malloc error");

    // pthread_mutex_lock不是取消点
    if (0 != (s = pthread_mutex_lock(&mtx)))
        perror("pthread_mutex_lock error");
    
    // clearup_handler被调用的时候：
    // 1, 线程调用pthread_exit()函数，而不是直接return
    // 2, 响应取消请求时，其它的线程对该线程调用pthread_cancel()
    // 3, 本线程调用pthread_cleanup_pop()函数，并且其参数非0
    pthread_cleanup_push(clearup_handler, buf);

    while (0 == glob)
    {
        // pthread_cond_wait是取消点
        if (0 != (s = pthread_cond_wait(&cond, &mtx)))
            perror("pthread_cond_wait error");
    }

    printf("thread: condition wait loop completed\n");
    // 当参数为0时，仅仅在线程调用pthread_exit函数或者其它线程对本线程调用 pthread_cancel函数时，才在弹出“清理函数”的同时执行该“清理函数”
    // pthread_cleanup_push()函数与pthread_cleanup_pop()函数必须成对的出现在同一个函数中
    // 调用return函数是不会在弹出“清理函数”的同时执行该“清理函数的
    pthread_cleanup_pop(1);
    
    return NULL;
}


/**
 *    
cc -g -Wall -o thread_clearup thread_clearup.c -lpthread

 ./thread_clearup 
main: about to cancel thread
clearup: freeing block at 0x7f91940008c0 
clearup: unlocking mutex
thread canceled successfully


 ./thread_clearup  1
main: about to signal condition variable
thread: condition wait loop completed
clearup: freeing block at 0x7ff0200008c0 
clearup: unlocking mutex
thread canceled failed

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

    // 第一种：取消线程方式
    if (argc == 1)
    {
        printf("main: about to cancel thread\n");

        if (0 != (s = pthread_cancel(t1)))
            perror("pthread_cancel error");
    }
    // 第二种：发送通知方式
    else
    {
        printf("main: about to signal condition variable\n");
        // 保护的不是同一段代码
        if (0 != (s = pthread_mutex_lock(&mtx)))
            perror("pthread_mutex_lock error");
        
        glob = 1;

        if (0 != (s = pthread_mutex_unlock(&mtx)))
            perror("pthread_mutex_unlock error");
        
        if (0 != (s = pthread_cond_signal(&cond)))
            perror("pthread_cond_signal error");
    }
    
    // 获取线程结束状态
    if (0 != (s = pthread_join(t1, &ret)))
            perror("pthread_join error");
    
    if (PTHREAD_CANCELED == ret)
        printf("thread canceled successfully\n");
    else 
        printf("thread canceled failed\n");

    exit(EXIT_SUCCESS);
}
