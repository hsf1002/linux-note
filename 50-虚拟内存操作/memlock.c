#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/mman.h>
#include <stddef.h>
#include <signal.h>
#include <ctype.h>
#include "get_num.h"

static void
display_mincore(char *addr, size_t len)
{
    unsigned char *vec;
    long page_size, num_pages;

    page_size = sysconf(_SC_PAGESIZE);

    num_pages = (len + page_size - 1) / page_size;

    if (NULL == (vec = malloc(num_pages)))
    {
        perror("malloc error");
        exit(EXIT_FAILURE);
    }

    if (-1 == mincore(addr, len, vec))
    {
        perror("mincore error");
        exit(EXIT_FAILURE);
    }

    for (int i=0; i<num_pages; i++)
    {
        if (i % 64 == 0)
            printf("%s%10p: ", (i == 0) ? "" : "\n", addr + (i * page_size));
        printf("%c", (vec[i] & 1) ? '*' : '.');
    }

    printf("\n");

    free(vec);
}

/**
 * 
 * 使用mincore获取内存驻留信息


cc -g -Wall -o memlock memlock.c libgetnum.so
./memlock 32 8 3
allocated 131072 (0x20000)bytes starting at 0x10c377000
before lock
0x10c377000: ................................
after lock
0x10c377000: ***.....***.....***.....***.....
 */
int main(int argc, char *argv[])
{
    char *addr;
    size_t len, lock_len;
    long page_size, step_size;

    if (argc != 4 || strcmp(argv[1], "--help") == 0)
    {
        fprintf(stderr, "%s num-pages lock-page-step lock-page-len\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (-1 == (page_size = sysconf(_SC_PAGESIZE)))
    {
        perror("pagesize error -1");
        exit(EXIT_FAILURE);
    }

    len = getInt(argv[1], GN_GT_0, "num-pages") * page_size;
    step_size = getInt(argv[2], GN_GT_0, "lock-page-step") * page_size;
    lock_len = getInt(argv[3], GN_GT_0, "lock-page-len") * page_size;

    // 创建共享匿名映射
    if (MAP_FAILED == (addr = mmap(NULL, len, PROT_READ, MAP_SHARED | MAP_ANONYMOUS, -1, 0)))
    {
        perror("mmap error");
        exit(EXIT_FAILURE);
    }

    printf("allocated %ld (%#lx)bytes starting at %p\n", (long)len, (unsigned long)len, addr);

    printf("before lock\n");
    display_mincore(addr, len);

    // 对映射的部分内容加锁
    for (int i=0; i+lock_len<=len; i+=step_size)
        if (-1 == mlock(addr + i, lock_len))
            perror("mlock error");

    printf("after lock\n");
    display_mincore(addr, len);

    exit(EXIT_SUCCESS);
}

