#define _GNU_SOURCE
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
#include <sched.h>
#include "get_num.h"

/**
 *    

 */
int
main(int argc, char *argv[])    
{
    pid_t pid;
    cpu_set_t set;
    unsigned long mask;

    if (argc != 3 || 0 == strcmp(argv[1], "--help"))
        fprintf(stderr, "%s pid\n", argv[0]);

    pid = getInt(argv[1], GN_NONNEG, "pid");
    mask = getLong(argv[2], GN_ANY_BASE, "octal-mask");

    CPU_ZERO(&set);

    for (int cpu=0; mask>0; cpu++, mask >>= 1)
    {
        if (mask & 1)
            CPU_SET(cpu, &set);
    }

    // 设置CPU亲和力
    if (-1 == sched_setaffinity(pid, sizeof(cpu_set_t), &set))
        perror("sched_setaffinity error");

    exit(EXIT_SUCCESS);
}
