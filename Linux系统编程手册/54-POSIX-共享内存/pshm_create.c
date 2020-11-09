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
#include "get_num.h"


static void
usage_error(const char *progName)
{
    fprintf(stderr, "Usage: %s [-cx] shm-name size [octal-perms]\n", progName);
    fprintf(stderr, "    -c   Create shared memory (O_CREAT)\n");
    fprintf(stderr, "    -x   Create exclusively (O_EXCL)\n");
    exit(EXIT_FAILURE);
}

/**
 * 创建一个POSIX共享内存对象
 * 
 */
int main(int argc, char *argv[])
{
    int flag, opt, fd;
    mode_t perms;
    size_t size;
    void *addr;

    flag = O_RDWR;

    while (-1 != (opt = getopt(argc, argv, "cx")))
    {
        switch (opt)
        {
            case 'c':
            {
                flag |= O_CREAT;
            }
            break;
            case 'x':
            {
                flag |= O_EXCL;
            }
            break;
            default:
            {
                usage_error(argv[0]);
            }
            break;
        }
    }

    if (optind + 1 >= argc)
    {
        usage_error(argv[0]);
    }

    size = getLong(argv[optind + 1], GN_ANY_BASE, "size");
    perms = (argc <= optind + 2) ? (S_IRUSR | S_IWUSR) : getLong(argv[optind + 2], GN_BASE_8, "octal-perms");

    // 创建共享内存
    if ( -1 == (fd = shm_open(argv[optind], flag, perms)))
    {
        perror("shm_open error");
        exit(EXIT_FAILURE);
    }

    // 设置大小
    if (-1 == ftruncate(fd, size))
    {
        perror("ftruncate error");
        exit(EXIT_FAILURE);
    }

    // 进行文件映射，一般不需要
//    if (MAP_FAILED == (addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)))
    {
  //      perror("mmap error");
    //    exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}

