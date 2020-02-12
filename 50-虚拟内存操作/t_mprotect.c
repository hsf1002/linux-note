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

#define LEN (1024 * 1024)

#define SHELL_FMT "cat /proc/%ld/maps | grep zero"
#define CMD_SIZE (sizeof(SHELL_FMT) + 20)

/**
 * 
 * 使用mprotect修改内存保护


./t_protect
before protect
7fc1bab74000-7fc1bac74000 ---s 00000000 00:05 76273                      /dev/zero (deleted)
after protect
7fc1bab74000-7fc1bac74000 rw-s 00000000 00:05 76273                      /dev/zero (deleted)
 */
int main(int argc, char *argv[])
{
    char *addr;
    char cmd[CMD_SIZE];


    // 创建共享匿名映射，拒绝所有访问
    if (MAP_FAILED == (addr = mmap(NULL, LEN, PROT_NONE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)))
    {
        perror("mmap error");
        exit(EXIT_FAILURE);
    }

    printf("before protect\n");
    snprintf(cmd, CMD_SIZE, SHELL_FMT, (long)getpid());
    system(cmd);

    // 对映射的保护更改为读写
    if (-1 == mprotect(addr, LEN, PROT_READ | PROT_WRITE))
    {
        perror("mprotect error");
        exit(EXIT_FAILURE);
    }

    printf("after protect\n");
    system(cmd);

    exit(EXIT_SUCCESS);
}

