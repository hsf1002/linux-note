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
 * 从一个POSIX共享内存对象读取数据
 * 
./pshm_create -c /demo_shm 0
ls -al /dev/shm
./pshm_write /demo_shm 'hello'
ls -l /dev/shm

./pshm_read /demo_shm 
 */
int main(int argc, char *argv[])
{
    int fd;
    struct stat sb;
    void *addr;

    if (argc != 2 || 0 == strcmp(argv[1], "--help"))
    {
        fprintf(stderr, "%s shm-name\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // 读写方式打开共享内存
    if ( -1 == (fd = shm_open(argv[1], O_RDONLY, 0)))
    {
        perror("shm_open error");
        exit(EXIT_FAILURE);
    }

    if (-1 == fstat(fd, &sb))
    {
        perror("fstat error");
        exit(EXIT_FAILURE);
    }

    // 进行文件映射
    if (MAP_FAILED == (addr = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0)))
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

    // 将共享内存的内容写到标准输出
    write(STDOUT_FILENO, addr, sb.st_size);
    
    exit(EXIT_SUCCESS);
}

