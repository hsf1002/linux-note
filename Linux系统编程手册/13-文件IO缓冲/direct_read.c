#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include "get_num.h"


/**
 *   使用O_DIRECT跳过缓冲区高速缓存
 * 
 */
int
main(int argc, char *argv[])    
{
    int fd;
    ssize_t num_read;
    size_t len;
    size_t aligment;
    off_t offset;
    void *buf;

    if (argc < 3 || 0 == strcmp(argc[1], "--help"))
        fprintf(stderr, "%s file lenght[offset [alignment]] \n", argv[0]);

    len = getLong(argv[2], GN_ANY_BASE, "length");
    offset = (argc > 3) ? getLong(argv[3], GN_ANY_BASE, "offset") : 0;
    aligment = (argc > 4) ? getLong(argv[4], GN_ANY_BASE, "aligment") : 4096;

    if (-1 == (fd = open(argv[1], O_RDONLY | O_DIRECT)))
        perror("open error");
    
    // 分配一块内存，其与第一个参数的整数倍对齐
    if (NULL == (buf = (char *)memalign(aligment * 2, len + aligment) + aligment))
        perror("memalign error");

    if (-1 == lseek(fd, offset, SEEK_SET))
        perror("lseek error");

    if (-1 == (num_read = read(fd, buf, len)))
        perror("read error");
    else
        printf("read %ld bytes \n", (long)num_read);

    exit(EXIT_SUCCESS);
}

