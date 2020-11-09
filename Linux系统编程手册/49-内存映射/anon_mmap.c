#ifdef USE_MAP_ANON
#define _GNU_SOURCE
#endif
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



/**
 * 在父子进程中共享一个匿名映射
 * 
macOS:
cc -g -Wall -o anon_mmap anon_mmap.c
./anon_mmap
mmap error: Operation not supported by device


Ubuntu:
./anon_mmap
child started, value = 1
parent, value = 2

 */
int main(int argc, char *argv[])
{
    char *addr;
    int fd;

#ifdef USE_MAP_ANON
    // 创建读写的共享文件映射
    if (MAP_FAILED == (addr = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)))
    {
        perror("mmap error");
        exit(EXIT_FAILURE);
    }
#else
    if (-1 == (fd = open("/dev/zero", O_RDWR)))
    {
        perror("open error");
        exit(EXIT_FAILURE);
    }

    if (MAP_FAILED == (addr = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)))
    {
        perror("mmap error");
        exit(EXIT_FAILURE);
    }

    if (-1 == close(fd))
    {
        perror("close error");
    }
#endif

    *addr = 1;

    switch (fork())
    {
        case -1:
        {
            perror("fork error");
            _exit(EXIT_FAILURE);
        }
        break;
        // 子进程将值加1
        case 0:
        {
            printf("child started, value = %d\n", *addr);

            (*addr)++;

            // 解除映射区域
            if (-1 == munmap(addr, sizeof(int)))
            {
                perror("munmap error");
            }
            exit(EXIT_SUCCESS);
        }
        break;
        // 父进程可以看到改变
        default:
        {
            if (-1 == wait(NULL))
            {
                perror("wait error");
            }

            printf("parent, value = %d\n", *addr);

            // 解除映射区域
            if (-1 == munmap(addr, sizeof(int)))
            {
                perror("munmap error");
            }
            exit(EXIT_SUCCESS);
        }
        break;
    }

    exit(EXIT_SUCCESS);
}

