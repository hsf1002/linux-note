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



/**
 * 通过私有文件映射，开发一个简易版的cat命令程序
 * 将命令行指定文件映射到内存，然后将内存中的内容写入标准输出
 
 ./a.out hello
hello world


 * 
 */
int main(int argc, char *argv[])
{
    char *addr;
    int fd;
    struct stat sb;

    if (argc != 2 || 0 == strcmp(argv[1], "--help"))
    {
        fprintf(stderr, "%s file\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // 读的方式打开命令行指定的文件
    if (-1 == (fd = open(argv[1], O_RDONLY)))
    {
        perror("open error");
        exit(EXIT_FAILURE);
    }

    if (-1 == fstat(fd, &sb))
    {
        perror("fstat error");
    }

    // 创建只读的私有文件映射
    if (MAP_FAILED == (addr = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0)))
    {
        perror("mmap error");
        exit(EXIT_FAILURE);
    }

    // 将映射中的内容写入标准输出
    if (sb.st_size != write(STDOUT_FILENO, addr, sb.st_size))
    {
        perror("write error");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}
