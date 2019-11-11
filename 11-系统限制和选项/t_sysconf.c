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
sysconf_print(const char *msg, int name)
{
    long lim;

    errno = 0;
    lim = sysconf(name);

    if (-1 != lim)
        printf("%s %ld\n", msg, lim);
    else
        if (0 == errno)
            printf("%s indeterminate\n", msg);
        else
            printf("sysconf %s", msg);
}

/**
 * 

_SC_ARG_MAX:         262144
_SC_LOGIN_NAME_MAX:  255
_SC_OPEN_MAX:        256
_SC_NGROUPS_MAX:     16
_SC_PAGESIZE:        4096
_SC_RTSIG_MAX:       indeterminate

*/
int
main(int argc, char *argv[])    
{
    sysconf_print("_SC_ARG_MAX:        ", _SC_ARG_MAX);
    sysconf_print("_SC_LOGIN_NAME_MAX: ", _SC_LOGIN_NAME_MAX);
    sysconf_print("_SC_OPEN_MAX:       ", _SC_OPEN_MAX);
    sysconf_print("_SC_NGROUPS_MAX:    ", _SC_NGROUPS_MAX);
    sysconf_print("_SC_PAGESIZE:       ", _SC_PAGESIZE);
    sysconf_print("_SC_RTSIG_MAX:      ", _SC_RTSIG_MAX);

    exit(EXIT_SUCCESS);
}

