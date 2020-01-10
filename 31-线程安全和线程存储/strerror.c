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

#define MAX_ERROR_LEN 256

static char buf[MAX_ERROR_LEN];


/**
 * strerr的非线程安全版本
 */
char *
strerror(int err)
{
    if (err < 0 || err >= _sys_nerr || _sys_errlist[err] == NULL)
    {
        snprintf(buf, MAX_ERROR_LEN, "unknown error %d", err);
    }
    else
    {
        strncpy(buf, _sys_errlist[err], MAX_ERROR_LEN - 1);
        buf[MAX_ERROR_LEN - 1] = '\0';
    }

    return buf;
}

/**
 * 
 */
static void *
thr_func(void *arg)
{
    char *str;

    printf("other thread about to call strerror()\n");
    str = strerror(EPERM);
    printf("other therad str(%p): %s\n", str, str);

    return NULL;
}

/**
 *    
main thread has called strerror()
other thread about to call strerror()
other therad str(0x6014e0): Operation not permitted
main thread str(0x6014e0): Operation not permitted


给变量buf加上修饰：__thread
main thread has called strerror()
other thread about to call strerror()
other therad str(0x7f771f164600): Operation not permitted
main thread str(0x7f771f947700): Invalid argument
* 
 */
int
main(int argc, char *argv[])    
{
    pthread_t t;
    int s;
    char *str;

    str = strerror(EINVAL);
    printf("main thread has called strerror()\n");

    if (0 != (s = pthread_create(&t, NULL, thr_func, NULL)))
        perror("pthread_create error");
    
    if (0 != (s = pthread_join(t, NULL)))
        perror("pthread_join error");
    
    printf("main thread str(%p): %s\n", str, str);

    exit(EXIT_SUCCESS);
}

