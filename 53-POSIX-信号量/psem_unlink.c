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
#include <sys/file.h>
#include <stddef.h>
#include <signal.h>
#include <ctype.h>
#include <semaphore.h>


/**
 * 
 * 删除一个POSIX命名信号量
 * 
 */
int main(int argc, char *argv[])
{
    if (argc != 2 || 0 == strcmp(argv[1], "--help"))
        fprintf(stderr, "% sem-name\n", argv[0]);
    
    if (-1 == unlink(argv[1]))
    {
        perror("unlink error");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}

