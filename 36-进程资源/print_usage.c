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
#include <sys/wait.h>
#include <sched.h>
#include "print_usage.h"


void
print_usage(const char *leader, const struct rusage *r)
{
    const char *ldr;

    ldr = (leader == NULL) ? "" : leader;

    printf("%s CPU time (sec):         user=%.3f, system=%.3f\n", ldr, r->ru_utime.tv_sec + r->ru_utime.tv_usec/1000000.0,
                                                                r->ru_stime.tv_sec + r->ru_stime.tv_usec/1000000.0);    
    printf("%s max resident set size:  %ld \n", ldr, r->ru_maxrss);
    printf("%s int shared memory:      %ld \n", ldr, r->ru_ixrss);
    printf("%s int unshared data:      %ld \n", ldr, r->ru_idrss);
    printf("%s int unshared stack:     %ld \n", ldr, r->ru_isrss);
    printf("%s page reclaims:          %ld \n", ldr, r->ru_minflt);
    printf("%s page faults:            %ld \n", ldr, r->ru_majflt);
    printf("%s swaps:                  %ld \n", ldr, r->ru_nswap);
    printf("%s block I/O:              input=%ld, output = %ld\n", ldr, r->ru_inblock, r->ru_oublock);
    printf("%s signals received:       %ld \n", ldr, r->ru_nsignals);
    printf("%s IPC messages:           sent=%ld, received=%ld\n", ldr, r->ru_msgsnd, r->ru_msgrcv);
    printf("%s context switches:       voluntary=%ld, involuntary=%ld\n", ldr, r->ru_nvcsw, r->ru_nivcsw);
}
