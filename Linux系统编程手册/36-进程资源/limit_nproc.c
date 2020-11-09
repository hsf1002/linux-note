#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/times.h>
#include <sched.h>
#include "print_limit.h"

/**
 *    
 */
int
main(int argc, char *argv[])    
{
    struct rlimit limit;
    pid_t pid;

    if (argc < 2 || argc > 3 || 0 == strcmp(argv[1], "--help"))
        printf(stderr, "%s soft-limit [hard-limit]\n", argv[0]);
    
    print_limit("init maximum process limit:   ", RLIMIT_NPROC);

    // 软限制
    limit.rlim_cur = (argv[1][0] == 'i') ? RLIM_INFINITY : getInt(argv[1], 0, "soft-limit");
    // 硬限制
    limit.rlim_max = (argc == 2) ? limit.rlim_cur: (argv[2][0] == 'i') ? RLIM_INFINITY : getInt(argv[2], 0, "hard-limit");

    // 设置可创建的进程的软硬限制
    if (-1 == setrlimit(RLIMIT_NPROC, &limit))
        perror("setrlimit error");

    print_limit("new maximum process limit:    ", RLIMIT_NPROC);

    // 创建尽可能多的进程
    for (int i=1; i<300; i++)
    {
        switch (pid = fork())
        {
            case -1:
                perror("fork error");
                exit(EXIT_FAILURE);

            case 0:
                _exit(EXIT_SUCCESS);
            
            default:
                printf("child %d (PID=%ld) started\n", i, (long)pid);
                break;
        }
    }
}
