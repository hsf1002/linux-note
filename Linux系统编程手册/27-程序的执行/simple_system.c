#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>

/*

*/
int simple_system(char *command)
{
    int status;
    pid_t child_pid;

    switch (child_pid = fork())
    {
    case -1:
        perror("fork error");
        break;
    case 0:
        execl("/bin/sh", "sh", "-c", command, (char *)NULL);
        _exit(127);
    default:
        if (-1 == waitpid(child_pid, &status, 0))
            return -1;
        else 
            return status;
    }
}

/**
 *   
 * 一个缺乏信号处理的system实现
 */
int
main(int argc, char *argv[])    
{
    printf(simple_system("uname"));
    
    exit(EXIT_SUCCESS);
}

