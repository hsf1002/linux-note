#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>



/**
 *   
 * 
// 输出到终端，默认行缓冲，所以printf的输出结果在前
skydeiMac:25-进程的终止 sky$ ./a.out 
Hello world!
skylotus

// 输出到终端，默认块缓冲，write的输出结果在前，printf要等刷新后才会显示
skydeiMac:25-进程的终止 sky$ ./a.out > a
skydeiMac:25-进程的终止 sky$ cat a
skylotus
Hello world!
Hello world!

 */
int
main(int argc, char *argv[])    
{
    // 方案1：关闭stdio缓冲
    // setbuf(stdout, NULL);

    // 库函数，stdio缓冲区->内核高速缓存->磁盘
    printf("Hello world!\n");
    // 系统调用，内核高速缓存->磁盘
    write(STDOUT_FILENO, "skylotus\n", 8);

    // 方案2：调用fork前强制刷新stdio缓冲区
    // fflush(stdout);

    // fork会复制进程用户空间的stdio缓冲区
    if (fork() == -1)
        perror("fork error\n");
    
    // 退出时会刷新父子进程的stdio缓冲区，导致重复的输出结果
    // 方案3：调用_exit，会导致Hello world无法写入文件
    // exit(EXIT_SUCCESS);
    _exit(EXIT_SUCCESS);
}

