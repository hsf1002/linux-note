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
LD_LIBRARY_PATH=. ./sched_getaffinity 4156
CPU:  0 1 2 3 4 5 6 7
LD_LIBRARY_PATH=. ./sched_getaffinity 3538
CPU:  0 1 2 3 4 5 6 7
LD_LIBRARY_PATH=. ./sched_getaffinity 3155
CPU:  0 1 2 3 4 5 6 7
sleep 10 &
[1] 7743
LD_LIBRARY_PATH=. ./sched_getaffinity 7743
CPU:  0 1 2 3 4 5 6 7


 */
int
main(int argc, char *argv[])    
{
    pid_t pid;
    cpu_set_t set;
    size_t s;

    if (argc != 2 || 0 == strcmp(argv[1], "--help"))
        fprintf(stderr, "%s pid\n", argv[0]);

    pid = getLong(argv[1], 0, "pid");

    // 获取CPU亲和力
    if (-1 == sched_getaffinity(pid, sizeof(cpu_set_t), &set))
        perror("sched_getaffinity error");

    printf("CPU: ");

    for (int cpu=0; cpu<CPU_SETSIZE; cpu++)
    {
        if (CPU_ISSET(cpu, &set))
            printf(" %d", cpu);
    }

    printf("\n");

    exit(EXIT_SUCCESS);
}



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
 sleep 50 &
[1] 10120
LD_LIBRARY_PATH=. ./sched_getaffinity 10120
CPU:  0 1 2 3 4 5 6 7

LD_LIBRARY_PATH=. ./sched_setaffinity 10120 0x1
LD_LIBRARY_PATH=. ./sched_getaffinity 10120
CPU:  0

LD_LIBRARY_PATH=. ./sched_setaffinity 10120 0x4
LD_LIBRARY_PATH=. ./sched_getaffinity 10120
CPU:  2

LD_LIBRARY_PATH=. ./sched_setaffinity 10120 0xe
LD_LIBRARY_PATH=. ./sched_getaffinity 10120
CPU:  1 2 3

LD_LIBRARY_PATH=. ./sched_setaffinity 10120 0xf
LD_LIBRARY_PATH=. ./sched_getaffinity 10120
CPU:  0 1 2 3

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

    // >>= 右移并赋值
    for (int cpu=0; mask>0; cpu++， mask >>= 1)
    {
        if (mask & 1)
            CPU_SET(cpu, &set);
    }

    // 设置CPU亲和力
    if (-1 == sched_setaffinity(pid, sizeof(cpu_set_t), &set))
        perror("sched_setaffinity error");

    exit(EXIT_SUCCESS);
}
