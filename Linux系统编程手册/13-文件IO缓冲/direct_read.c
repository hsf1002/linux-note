#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include "get_num.h"

#define BUF_SIZE 40960


/**
 *   使用O_DIRECT跳过缓冲区高速缓存
 * 
 * cc direct_read.c -o direct_read libgetnum.so

./direct_read ./get_num.h 100
read error: Bad address

./direct_read ./README.md 100
read error: Invalid argument

-----------------read的len改为BUF_SIZE后：
 ./direct_read ./testfile.txt 1
read 4096 bytes, size = 55, buf = to take on an issue, a complex issue, a deep inequality 

./direct_read ./get_num.h 1
read 527 bytes, size = 527, buf = #ifndef GET_NUM_H
#define GET_NUM_H

#define GN_NONNEG       01      /* Value must be >= 0 * /
#define GN_GT_0         02      /* Value must be > 0 * /

/* By default, integers are decimal * /
#define GN_ANY_BASE   0100      /* Can use any base - like strtol(3) * /
#define GN_BASE_8     0200      /* Value is expressed in octal * /
#define GN_BASE_16    0400      /* Value is expressed in hexadecimal * /

long getLong(const char *arg, int flags, const char *name);
int getInt(const char *arg, int flags, const char *name);

#endif

 */
int
main(int argc, char *argv[])    
{
    int fd;
    ssize_t num_read;
    size_t len;
    size_t aligment;
    off_t offset;
    char * buf;

    if (argc < 3 || 0 == strcmp(argv[1], "--help"))
        fprintf(stderr, "%s file lenght[offset [alignment]] \n", argv[0]);

    len = getLong(argv[2], GN_ANY_BASE, "length");
    offset = (argc > 3) ? getLong(argv[3], GN_ANY_BASE, "offset") : 0;
    aligment = (argc > 4) ? getLong(argv[4], GN_ANY_BASE, "aligment") : 4096;

    if (-1 == (fd = open(argv[1], O_RDONLY|O_DIRECT)))
        perror("open error");
    
#if 1
    if ( 0 != posix_memalign(( void **)&buf, 4096, BUF_SIZE ))
    {
        perror("posix_memalign error");
    }
#else
    // 分配一块内存，其与第一个参数的整数倍对齐
    if (NULL == (buf = memalign(aligment * 2, len + aligment)))
        perror("memalign error");

    buf += aligment;
#endif
    if (-1 == lseek(fd, offset, SEEK_SET))
        perror("lseek error");

    //if (-1 == (num_read = read(fd, buf, len)))
    if (-1 == (num_read = read(fd, buf, BUF_SIZE)))
        perror("read error");
    else
    {
        printf("read %ld bytes, size = %d, buf = %s \n", (long)num_read, strlen(buf), buf);
    }

    exit(EXIT_SUCCESS);
}

