#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ftw.h>

#define BUF_SIZE PATH_MAX
 

/**
 *   
 * 
skydeiMac:18-目录与链接 sky$ ln -s README.md x
skydeiMac:18-目录与链接 sky$ ./a.out x
readlink: x ---> README.md
realpath: x ---> /Users/sky/work/practice/linux-note/18-目录与链接/README.md 
 
 */
int
main(int argc, char *argv[])    
{
    struct stat stat_buf;
    char buf[BUF_SIZE];
    ssize_t num_bytes;

    if (argc != 2 || 0 == strcmp(argv[1], "--help"))
        fprintf(stderr, "%s pathname\n", argv[0]);
    
    if (-1 == lstat(argv[1], &stat_buf))
        perror("lstat error");
    
    if (!S_ISLNK(stat_buf.st_mode))
        fprintf(stderr, "%s is not a symbolic file", argv[1]);
    
    // 获取符号链接的文件名
    if (-1 == (num_bytes = readlink(argv[1], buf, BUF_SIZE -1)))
        perror("readlink error");
    
    buf[num_bytes] = '\0';

    printf("readlink: %s ---> %s\n", argv[1], buf);

    // 获取符号链接的绝对路径
    if (NULL == realpath(argv[1], buf))
        perror("realpath error");
    
    printf("realpath: %s ---> %s\n", argv[1], buf);

    exit(EXIT_SUCCESS);
}

