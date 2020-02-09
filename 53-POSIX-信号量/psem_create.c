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
#include "get_num.h"


static void
usage_error(const char *progName)
{
    fprintf(stderr, "Usage: %s [-cx] name [octal-perms [value]]\n", progName);
    fprintf(stderr, "    -c   Create semaphore (O_CREAT)\n");
    fprintf(stderr, "    -x   Create exclusively (O_EXCL)\n");
    exit(EXIT_FAILURE);
}



/**
 * 
创建一个POSIX命令信号量

umask 007
./psem_create -cx /demo 666
ls -l /dev/shm/sem.*

./psem_create -cx /demo 666
sem_open create error: File exists
 */
int main(int argc, char *argv[])
{
    int flag, opt;
    mode_t perms;
    unsigned int value;
    sem_t *sem;

    flag = 0;

    while (-1 != (opt = getopt(argc, argv, "cx")))
    {
        switch (opt)
        {
            case 'c':
                flag |= O_CREAT;
            break;
            case 'x':
                flag |= O_EXCL;
            break;
            default:
                usage_error(argv[0]);
            break;
        }
    }

    if (optind >= argc)
        usage_error(argv[0]);
    
    perms = (argc <= optind + 1) ? (S_IRUSR | S_IWUSR) : getInt(argv[optind + 1], GN_BASE_8, "octal-perms");
    value = (argc <= optind + 2) ? 0 : getInt(argv[optind + 2], 0, "value");

    // 创建信号量
    if (SEM_FAILED == (sem = sem_open(argv[optind], flag, perms, value)))
    {
        perror("sem_open create error");
        exit(EXIT_FAILURE);
    }    

    exit(EXIT_SUCCESS);
}

