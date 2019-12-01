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
 *   
 * 
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

