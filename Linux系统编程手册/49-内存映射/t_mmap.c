//#define _GNU_SOURCE
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


#define MEM_SIZE 10

/**
 * 创建一个共享文件映射
 * 
cc -g -Wall -o t_mmap t_mmap.c 
dd if=/dev/zero of=s.txt bs=1 count=1024
1024+0 records in
1024+0 records out
1024 bytes transferred in 0.002707 secs (378278 bytes/sec)

cat s.txt 

./t_mmap s.txt 'hello world'
current string=
new value too large: Undefined error: 0

./t_mmap s.txt 'hi world'
current string=
copied [hi world] to the shared memory

./t_mmap s.txt 'hi sky'
current string=hi world
copied [hi sky] to the shared memory

skydeiMac:49-内存映射 sky$ cat s.txt 
hi sky 
* 
 */
int main(int argc, char *argv[])
{
    char *addr;
    int fd;

    if (argc < 2 || 0 == strcmp(argv[1], "--help"))
    {
        fprintf(stderr, "%s file [new-value] \n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // 读写方式打开命令行指定的文件
    if (-1 == (fd = open(argv[1], O_RDWR)))
    {
        perror("open error");
        exit(EXIT_FAILURE);
    }

    // 创建读写的共享文件映射
    if (MAP_FAILED == (addr = mmap(NULL, MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)))
    {
        perror("mmap error");
        exit(EXIT_FAILURE);
    }

    // 已经不需要了
    if (-1 == close(fd))
    {
        perror("close error");
    }

    printf("current string=%.*s\n", MEM_SIZE, addr);

    // 如果是更新映射中内容
    if (argc > 2)
    {
        if (strlen(argv[2]) > MEM_SIZE)
        {
            perror("new value too large");
            exit(EXIT_FAILURE);
        }

        memset(addr, 0, MEM_SIZE);

        strncpy(addr, argv[2], MEM_SIZE - 1);

        // 同步映射区域
        if (-1 == msync(addr, MEM_SIZE, MS_SYNC))
        {
            perror("msync error");
        }

        printf("copied [%s] to the shared memory\n", addr);
    }

    exit(EXIT_SUCCESS);
}

