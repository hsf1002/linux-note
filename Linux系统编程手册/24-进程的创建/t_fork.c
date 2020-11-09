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

static int sdata = 111;


/**
 *   
 * 
 ./a.out 
pid = 92667, child, idata = 444, sdata = 222
pid = 92666, parent, idata = 222, sdata = 111
 */
int
main(int argc, char *argv[])    
{
    int idata = 222;
    pid_t child_pid;

    switch (child_pid = fork())
    {
    // error
    case -1:
        perror("fork error");
        break;
    // 子进程
    case 0:
        idata *= 2;
        sdata *= 2;
        break;
    // 父进程
    default:
        // 让子进程先运行
        sleep(3);
        break;
    }
    // 父子进程都会走到这里
    printf("pid = %ld, %s, idata = %d, sdata = %d\n", getpid(), (child_pid == 0) ? "child" : "parent", idata, sdata);
    
    exit(EXIT_SUCCESS);
}

