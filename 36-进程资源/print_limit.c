#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/times.h>
#include <sched.h>
#include "print_limit.h"

/**
 */
int
print_limit(const char *msg, int resouce)
{
    struct rlimit limit;

    if (-1 == getrlimit(resouce, &limit))
        return -1;

    printf("%s soft = ", msg);

    if (RLIM_INFINITY == limit.rlim_cur)
        printf("infinit");
#ifdef RLIM_SAVED_CUR        
    else if (RLIM_SAVED_CUR == limit.rlim_cur)
        printf("unrepresentable");
#endif        
    else
        printf("%lld", (long long)limit.rlim_cur);
    
    printf(", hard = ");

    if (RLIM_INFINITY == limit.rlim_max)
        printf("infinit");
#ifdef RLIM_SAVED_CUR        
    else if (RLIM_SAVED_CUR == limit.rlim_max)
        printf("unrepresentable");
#endif        
    else
        printf("%lld\n", (long long)limit.rlim_max);
    return 0;
}
