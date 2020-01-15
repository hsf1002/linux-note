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
static pthread_mutex_t mtx;
static pthread_spinlock_t splock;
static int use_mutex = 0;
static int num_outer_loops = 0;
static int num_inner_lopps = 0;


/**
 * 
 */
static void *
thr_func(void *arg)
{
    int s;

    for (int i=0; i<num_outer_loops; i++)
    {
        if (use_mutex)
        {
            if (0 != (s = pthread_mutex_lock(&mtx)))
                perror("pthread_mutex_lock error");
        }
        else
        {
            if (0 != (s = pthread_spin_lock(&splock)))
                perror("pthread_spin_lock error");
        }
        
        for (int k=0; k<num_inner_lopps; k++)        
        {
            glob++;
        }

        if (use_mutex)
        {
            if (0 != (s = pthread_mutex_unlock(&mtx)))
                perror("pthread_mutex_unlock error");
        }
        else
        {
            if (0 != (s = pthread_spin_unlock(&splock)))
                perror("pthread_spin_unlock error");
        } 
    }

    return NULL;
}

/**
 * 
 */
static void 
usage_error(char *pname)
{
    fprintf(stderr, "usage: %s [-s] num-threads [num-inner-loops [num-outer-loops]])\n", pname);
    fprintf(stderr, "    -q    Don't print verbose messages\n");
    fprintf(stderr, "    -s    use spin locks(instead of the default mutex)\n");
    
    exit(EXIT_FAILURE);
}

/**
 *    

./thread_lock_speed 5
using mutex 
         threads: 5, outer loops: 100000, inner loops: 1 
main thread glob: 500000
./thread_lock_speed 4
using mutex 
         threads: 4, outer loops: 100000, inner loops: 1 
main thread glob: 400000
./thread_lock_speed -s 5
using spin lock 
         threads: 5, outer loops: 100000, inner loops: 1 
main thread glob: 500000
./thread_lock_speed -s 4
using spin lock 
         threads: 4, outer loops: 100000, inner loops: 1 
main thread glob: 400000

 */
int
main(int argc, char *argv[])    
{
    pthread_t *threads;
    int num_threads;
    int s;
    int opt;
    // 是否打印信息
    int verbose;

    alarm(120);

    use_mutex = 1;
    verbose = 1;

    // 命令行参数可以传-q -s
    while((opt = getopt(argc, argv, "qs")) != -1)
    {
        switch (opt)
        {
            case 'q':
                verbose = 0;
            break;
            case 's':
                use_mutex = 0;
            break;
            default:
                usage_error(argv[0]);
            break;
        }
    }

    // optind表示选项参数的索引，循环之后，optint指向"qs"这个选项字符串的后面的参数
    if (optind >= argc)
        usage_error(argv[0]);
    // 如./thread_lock_speed -s 4，optind的值是2, argv[optind]是4
    num_threads = atoi(argv[optind]);
    num_inner_lopps = (optind + 1 < argc) ? atoi(argv[optind + 1]) : 1;
    num_outer_loops = (optind + 2 < argc) ? atoi(argv[optind + 2]) : 100000;

    if (verbose)
    {
        printf("using %s \n", use_mutex ? "mutex" : "spin lock");
        printf("\t\t threads: %d, outer loops: %d, inner loops: %d \n", num_threads, num_outer_loops, num_inner_lopps);
    }

    if (NULL == (threads = calloc(num_threads, sizeof(pthread_t))))
        perror("calloc error");
    
    if (use_mutex)
    {
        if (0 != (s = pthread_mutex_init(&mtx, NULL)))
            perror("pthread_mutex_init error");
    }
    else
    {
        if (0 != (s = pthread_spin_init(&splock, NULL)))
            perror("pthread_spin_init error");
    }  

    for (int i=0; i<num_threads; i++)
    {
        if (0 != (s = pthread_create(&threads[i], NULL, thr_func, NULL)))
            perror("pthread_create error");
    }
   
    for (int i=0; i<num_threads; i++)
    {
        if (0 != (s = pthread_join(threads[i], NULL)))
            perror("pthread_join error");
    }
    
    if (verbose)
        printf("main thread glob: %d\n", glob);

    exit(EXIT_SUCCESS);
}
