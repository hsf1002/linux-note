//#define _GNU_SOURCE
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
#include <sys/shm.h>
#include <stddef.h>
#include <signal.h>
#include <ctype.h>
#include "get_num.h"


static void
usage_error(const char *progName, const char *msg)
{
    if (msg != NULL)
        fprintf(stderr, "%s", msg);
    fprintf(stderr, "Usage: %s [-cx] {-f pathname | -k key | -p} "
                            "seg-size [octal-perms]\n", progName);
    fprintf(stderr, "    -c           Use IPC_CREAT flag\n");
    fprintf(stderr, "    -x           Use IPC_EXCL flag\n");
    fprintf(stderr, "    -f pathname  Generate key using ftok()\n");
    fprintf(stderr, "    -k key       Use 'key' as key\n");
    fprintf(stderr, "    -p           Use IPC_PRIVATE key\n");
    exit(EXIT_FAILURE);
}


/**
 * 命令行参数创建System V共享内存
 * 
 */
int main(int argc, char *argv[])
{
    int num_key_flags;
    int flags, shm_id, seg_size, opt;
    unsigned int perms;
    long lkey;
    key_t key;

    num_key_flags = 0;
    flags = 0;

    while (-1 != (opt = getopt(argc, argv, "cf:k:px")))
    {
        switch (opt)
        {
            case 'c':
            {
                flags |= IPC_CREAT;
            }
            break;
            case 'f':
            {
                if (-1 == (key = ftok(optarg, 1)))
                {
                    perror("ftok error");
                    exit(EXIT_FAILURE);
                }
                num_key_flags++;
            }
            break;
            case 'k':
            {
                if (1 != sscanf(optarg, "%li", &lkey))
                {
                    perror("-k option requires a numberic argument");
                    exit(EXIT_FAILURE);
                }
                key = lkey;
                num_key_flags++;
            }
            break;
            case 'p':
            {
                flags |= IPC_PRIVATE;
                num_key_flags++;
            }
            break;
            case 'x':
            {
                flags |= IPC_EXCL;
            }
            break;
            default:
                usage_error(argv[0], NULL);
            break;
        }
    }

    if (1 != num_key_flags)
    {
        fprintf(stderr, "exactly one of the options [-f]/[-k]/[-p] must be supplied");
        exit(EXIT_FAILURE);
    }

    if (optind >= argc)
    {
        perror("must specify number of semaphores");
        exit(EXIT_FAILURE);
    }

    seg_size = getInt(argv[optind], 0, "seg-size");

    perms = (argc <= optind + 1) ? (S_IRUSR | S_IWUSR) : getInt(argv[optind + 1], GN_BASE_8, "octal-perms");

    if (-1 == (shm_id = shmget(key, seg_size, flags | perms)))
    {
        perror("shmget create error");
        exit(EXIT_FAILURE);
    }

    printf("shared memory created shmid = %d \n", shm_id);

    exit(EXIT_SUCCESS);
}

