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
 * 为文件描述符设置执行时关闭标志
 
skydeiMac:27-程序的执行 sky$ cc closeonexec.c -o closeonexec
skydeiMac:27-程序的执行 sky$ ./closeonexec 
-rwxr-xr-x  1 sky  staff  8568 12 28 19:41 ./closeonexec
skydeiMac:27-程序的执行 sky$ ./closeonexec n


 */
int
main(int argc, char *argv[])    
{
    int flag;

    if (argc > 1)
    {
        if (-1 == (flag = fcntl(STDOUT_FILENO, F_GETFD)))
            perror("fcntl get fd error");

        flag |= FD_CLOEXEC;

        if (-1 == fcntl(STDOUT_FILENO, F_SETFD, flag))
            perror("fcntl set fd error");
    }
    
    execlp("ls", "ls", "-l", argv[0], (char *)NULL);

    perror("execl error");
    
    exit(EXIT_SUCCESS);
}

