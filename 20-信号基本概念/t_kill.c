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
#include <signal.h>
#include "get_num.h"



/**
 *   ps|grep cat
16258 pts/22   00:00:00 catch_rtsigs
22235 pts/22   00:00:00 catch_rtsigs

LD_LIBRARY_PATH=. ./t_kill 16258 15 (SIGTERM)
./catch_rtsigs: sleep completed
ps|grep cat
22235 pts/22   00:00:00 catch_rtsigs
[3]+  已完成               LD_LIBRARY_PATH=. ./catch_rtsigs 100

LD_LIBRARY_PATH=. ./t_kill 22235 19 (SIGSTOP)
[1]+  已停止               LD_LIBRARY_PATH=. ./catch_rtsigs 60
ps|grep cat
22235 pts/22   00:00:00 catch_rtsigs
LD_LIBRARY_PATH=. ./t_kill 22235 18 (SIGCONT)
caught signal: 18
, s_signo = 18, s_code = 0 (SI_USER), s_value = 586486120
    s_pid = 16366, s_uid = 1000 
ps|grep cat
22235 pts/22   00:00:00 catch_rtsigs
LD_LIBRARY_PATH=. ./t_kill 22235 15 (SIGTERM)
ps|grep cat
[1]+  已完成               LD_LIBRARY_PATH=. ./catch_rtsigs 60
ps|grep cat
 */
int
main(int argc, char *argv[])    
{
    int s;
    int sig;

    if (argc != 3 && 0 == strcmp(argv[1], "--help"))
        fprintf(stderr, "%s sig-no pid \n", argv[0]);
    
    sig = getInt(argv[2], 0, "sig_no");

    s = kill(getLong(argv[1], 0, "pid"), sig);

    if (sig != 0)
    {
        if (-1 == s)
            perror("kill error");
    }
    else
    {
        if (0 == s)
            printf("process exists and we can send it a signal");
        else
        {
            if (errno == EPERM)
                printf("process exists, but we don't have permission to send a signal");
            else if (ESRCH == errno)
                printf("process does not exists");
            else
                perror("kill error");
        }
    }

    exit(EXIT_SUCCESS);
}
