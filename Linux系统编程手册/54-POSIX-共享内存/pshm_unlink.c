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


/**
 * 断开链接一个POSIX共享内存对象
 * 
 */
int main(int argc, char *argv[])
{

    if (argc != 2 || 0 == strcmp(argv[1], "--help"))
    {
        fprintf(stderr, "%s shm-name\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (-1 == shm_unlink(argv[1]))
    {
        perror("shm_unlink error");
        exit(EXIT_FAILURE);
    }
    
    exit(EXIT_SUCCESS);
}

