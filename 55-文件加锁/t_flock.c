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
#include "get_num.h"

/**
 * 使用flock
 * 
 

cc -g -Wall -o t_flock t_flock.c libgetnum.so 
./t_flock tfile s 60 &
[3] 45844
./t_flock tfile s 2
PID=45846, requesting LOCK_SH
PID=45846, granted LOCK_SH
PID=45846, releasing LOCK_SH

[3]+  Stopped                 ./t_flock tfile s 60

./t_flock tfile xn
PID=45847, requesting LOCK_EX
PID=45847, granted LOCK_EX
PID=45847, releasing LOCK_EX

./t_flock tfile x
PID=45849, requesting LOCK_EX
PID=45849, granted LOCK_EX
PID=45849, releasing LOCK_EX 
 */
int main(int argc, char *argv[])
{
    int fd, lock;
    const char *lname;

    if (argc < 3 || strcmp(argv[1], "--help") == 0 || strchr("sx", argv[2][0]) == NULL)
    {
        fprintf(stderr, "%s file lock [sleep-time]\n"
                 "    'lock' is 's' (shared) or 'x' (exclusive)\n"
                 "        optionally followed by 'n' (nonblocking)\n"
                 "    'sleep-time' specifies time to hold lock\n", argv[0]);
    
        exit(EXIT_FAILURE);
    }

    lock = (argv[2][0] == 's') ? LOCK_SH : LOCK_EX;
    if (argv[2][1] == 'n')
        lock |= LOCK_NB; // nonblock

    // 打开文件
    if (-1 == (fd = open(argv[1], O_RDONLY)))
    {
        perror("open error");
        exit(EXIT_FAILURE);
    }

    lname = (lock & LOCK_SH) ? "LOCK_SH" : "LOCK_EX";
    printf("PID=%ld, requesting %s\n", (long)getpid(), lname);

    // 对其加锁
    if (-1 == flock(fd, lock))
    {
        if (errno == EWOULDBLOCK)
        {
            fprintf(stderr, "PID=%ld already locked, bye!", (long)getpid());
            exit(EXIT_FAILURE);
        }
        else
        {
            perror("some unexpected error");
            exit(EXIT_FAILURE);
        }
    }
    printf("PID=%ld, granted %s\n", (long)getpid(), lname);

    sleep((argc > 3) ? getInt(argv[3], GN_NONNEG, "sleep-time") : 10);

    printf("PID=%ld, releasing %s\n", (long)getpid(), lname);

    // 对其解锁
    if (-1 == flock(fd, LOCK_UN))
    {
        perror("flock unlock error");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}

