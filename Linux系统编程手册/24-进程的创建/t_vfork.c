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
#include <sys/wait.h>

/**
 *   
 * 
 ./a.out 
child executing
parent executing
parent data: 222
 */
int
main(int argc, char *argv[])    
{
    int data = 111;
    char child_str[] = "child executing\n";
    char parent_str[] = "parent executing\n";
    int size;

    switch (vfork())
    {
    // error
    case -1:
        perror("vfork error");
        break;
    // 子进程
    case 0:
        // 即使睡眠3秒，也先执行
        sleep(3);
        size = strlen(child_str);
        write(STDOUT_FILENO, child_str, size + 1);
        data *= 2;

        _exit(EXIT_SUCCESS);
    // 父进程
    default:
        size = strlen(parent_str);
        write(STDOUT_FILENO, parent_str, size + 1);
       
        // 子进程并不复制数据段，该值被修改了
        printf("parent data: %d\n", data);

        exit(EXIT_SUCCESS);
    }
}

