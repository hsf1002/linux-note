
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
static int
thr_fun(void *arg)
{
    char *s = (char *)arg;

    printf("%s", s);

    return (void *)strlen(s);
}

/**
 *   
main message
Hello world
main returned 12
* 
 */
int
main(int argc, char *argv[])    
{
    pthread_t t;
    void *res;
    int s;

    if (0 != (s = pthread_create(&t, NULL, thr_fun, "Hello world\n")))
        perror("pthread_create error");
    
    // 和"Hello world"的打印顺序取决于系统调度
    printf("main message\n");

    if (0 != (s = pthread_join(t, &res)))
        perror("pthread_join error");
    
    printf("main returned %ld \n", (long)res);

    exit(EXIT_SUCCESS);
}

