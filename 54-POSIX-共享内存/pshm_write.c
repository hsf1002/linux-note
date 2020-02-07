//#define _BSD_SOURCE
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
 * 将数据写到一个POSIX共享内存对象
 * 
 */
int main(int argc, char *argv[])
{
    int fd;
    size_t size;
    void *addr;

    if (argc != 3 || 0 == strcmp(argv[1], "--help"))
    {
        fprintf(stderr, "%s shm-name string\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // 读写方式打开共享内存
    if ( -1 == (fd = shm_open(argv[1], O_RDWR, 0)))
    {
        perror("shm_open error");
        exit(EXIT_FAILURE);
    }

    size = strlen(argv[2]);

    // 设置大小
    if (-1 == ftruncate(fd, size))
    {
        perror("ftruncate error");
        exit(EXIT_FAILURE);
    }

    printf("shared memory resized to %ld bytes\n", (long)size);

    // 进行文件映射
    if (MAP_FAILED == (addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)))
    {
        perror("mmap error");
        exit(EXIT_FAILURE);
    }

    // 不再需要
    if (-1 == close(fd))
    {
        perror("close error");
        exit(EXIT_FAILURE);
    }

    memcpy(addr, argv[2], size);

    printf("now shared memory addr = %s\n", (char *)addr);
    
    exit(EXIT_SUCCESS);
}

