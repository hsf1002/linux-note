#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>



/**
 *  用errno区分无法确定限制值和出错两种情况
 */
static void
fpathconf_print(const char *msg, int fd, int name)
{
    long lim;

    errno = 0;
    lim = fpathconf(fd, name);

    if (-1 != lim)
        printf("%s %ld\n", msg, lim);
    else
        if (0 == errno)
            printf("%s indeterminate\n", msg);
        else
            printf("%s not found\n", msg);
}

/**

_PC_NAME_MAX:         not found
_PC_PATH_MAX:         not found
_PC_PIPE_BUF:         512
* 
 */
int
main(int argc, char *argv[])    
{
    fpathconf_print("_PC_NAME_MAX:        ", STDIN_FILENO, _PC_NAME_MAX);
    fpathconf_print("_PC_PATH_MAX:        ", STDIN_FILENO, _PC_PATH_MAX);
    fpathconf_print("_PC_PIPE_BUF:        ", STDIN_FILENO, _PC_PIPE_BUF);

    exit(EXIT_SUCCESS);
}

