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
#include <sched.h>
#include "get_num.h"

/**
 *    
sleep 30 &
[1] 5328
LD_LIBRARY_PATH=. ./sched_view 5328
5328: OTHER 0 

 */
int
main(int argc, char *argv[])    
{
    int pol;
    struct sched_param sp;

    for (int i=1; i<argc; i++)
    {
        // 获取策略
        if (-1 == (pol = sched_getscheduler(getLong(argv[i], 0, "pid"))))
            perror("sched_getscheduler error");

        // 获取优先级
        if (-1 == sched_getparam(getLong(argv[i], 0, "pid"), &sp))
            perror("sched_getparam error");
        
        printf("%s: %-5s", argv[i], (pol == SCHED_OTHER) ? "OTHER" :
                                    (pol == SCHED_RR) ? "RR" :
                                    (pol == SCHED_FIFO) ? "FIFO" :
                                #ifdef SCHED_BATCH
                                    (pol == SCHED_BATCH) ? "BATCH" :
                                #endif
                                #ifdef SCHED_IDLE
                                    (pol == SCHED_IDLE) ? "IDLE" :
                                #endif        
                                    "???"
                                    );
        printf("%2d \n", sp.__sched_priority);                                    
    }

    exit(EXIT_SUCCESS);
}
