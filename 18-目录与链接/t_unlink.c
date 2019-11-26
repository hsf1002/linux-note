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
#include "get_num.h"

#define CMD_SIZE 200
#define BUF_SIZE 1024



/**
 *   
 * 
cc -g -Wall -o t_unlink t_unlink.c libgetnum.so 
LD_LIBRAY_PATH=:. ./t_unlink hello
df: dirname hello: No such file or directory
************ close file descriptor 
df: dirname hello: No such file or directory

 */
int
main(int argc, char *argv[])    
{
    int fd;
    int num_blocks;
    char shell_cmd[CMD_SIZE];
    char buf[BUF_SIZE];

    if (argc < 2 || 0 == strcmp(argv[1], "--help"))
        fprintf(stderr, "%s temp-file [num-1kb-blocks] \n", argv[0]);
    
    num_blocks = (argc > 2) ? getInt(argv[2], GN_GT_0, "num-1kb-block") : 100000;

    // 创建一个文件
    if (-1 == (fd = open(argv[1], O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR)))
        perror("fopen error");

    // 将其文件名删除（文件本身存在）
    if (-1 == unlink(argv[1]))
        perror("unlink error");
    
    // 向其写入内容
    for (int i=0; i<num_blocks; i++)
        if (write(fd, buf, BUF_SIZE) != BUF_SIZE)
            perror("partial write failed");

    // 查看文件占有磁盘空间
    snprintf(shell_cmd, CMD_SIZE, "df -k 'dirname %s'", argv[1]);
    system(shell_cmd);
    
    // 关闭文件
    if (-1 == close(fd))
        perror("close error");
    
    printf("************ close file descriptor \n");

    // 再次查看文件占有磁盘空间
    system(shell_cmd);

    exit(EXIT_SUCCESS);
}

