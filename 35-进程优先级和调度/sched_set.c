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

sleep 120 &
[2] 7172
LD_LIBRARY_PATH=. ./sched_view 7172
7172: OTHER 0 

LD_LIBRARY_PATH=. ./sched_set r 10 7172
sched_setscheduler error: Operation not permitted

sudo LD_LIBRARY_PATH=. ./sched_set r -10 7172
sched_setscheduler error: Invalid argument

sudo LD_LIBRARY_PATH=. ./sched_set r 10 7172
LD_LIBRARY_PATH=. ./sched_view 7172
7172: RR   10 


sleep 120 &
[3] 7219
LD_LIBRARY_PATH=. ./sched_view 7219
7219: OTHER 0 
sudo LD_LIBRARY_PATH=. ./sched_set r 10 7219
LD_LIBRARY_PATH=. ./sched_view 7219
7219: RR   10 

sudo LD_LIBRARY_PATH=. ./sched_set f -10 7219
sched_setscheduler error: Invalid argument

sudo LD_LIBRARY_PATH=. ./sched_set r -10 7219
sched_setscheduler error: Invalid argument

sudo LD_LIBRARY_PATH=. ./sched_set f 2 7219
LD_LIBRARY_PATH=. ./sched_view 7219
7219: FIFO  2 


 */
int
main(int argc, char *argv[])    
{
    int pol;
    struct sched_param sp;
    if (argc < 3 || strchr("rfo"
#ifdef SCHED_BATCH              /* Linux-specific */
                "b"
#endif
#ifdef SCHED_IDLE               /* Linux-specific */
                "i"
#endif
                , argv[1][0]) == NULL)
        fprintf(stderr, "%s policy priority [pid...]\n"
                "    policy is 'r' (RR), 'f' (FIFO), "
#ifdef SCHED_BATCH              /* Linux-specific */
                "'b' (BATCH), "
#endif
#ifdef SCHED_IDLE               /* Linux-specific */
                "'i' (IDLE), "
#endif
                "or 'o' (OTHER)\n",
                argv[0]);

    pol = (argv[1][0] == 'r') ? SCHED_RR :
          (argv[1][0] == 'f') ? SCHED_FIFO :
#ifdef SCHED_BATCH              /* Linux-specific, since kernel 2.6.16 */
          (argv[1][0] == 'b') ? SCHED_BATCH :
#endif
#ifdef SCHED_IDLE               /* Linux-specific, since kernel 2.6.23 */
          (argv[1][0] == 'i') ? SCHED_IDLE :
#endif
          SCHED_OTHER;

    sp.__sched_priority = getInt(argv[2], 0, "priority");

    for (int i=3; i<argc; i++)
    {
        // 设置策略和优先级
        if (-1 == sched_setscheduler(getLong(argv[i], 0, "pid"), pol, &sp))
            perror("sched_setscheduler error");
    }

    exit(EXIT_SUCCESS);
}
