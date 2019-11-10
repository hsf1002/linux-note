#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <limits.h>
#include <time.h>
#include <sys/times.h>
#include <locale.h>
#include "get_num.h"



/**
 * 
 */
static void
display_process_time(const char *msg)
{
    struct tms t;
    clock_t clock_time;
    static long clock_ticks = 0;

    if (NULL != msg)
        printf("%s", msg);
    
    if (0 == clock_ticks)
        if (-1 == (clock_ticks = sysconf(_SC_CLK_TCK)))
            perror("sysconf error");
    if (-1 == (clock_time = clock()))
        perror("clock_time error");

    printf("  clock return: %ld clocks-per-sec (%.2f secs)\n", (long)clock_time, (double)clock_time/CLOCKS_PER_SEC);

    if (-1 == times(&t))
        perror("times error");
    
    printf("  times yields: user CPU: %.2f, system CPU: %.2f \n", (double)t.tms_utime/clock_ticks, (double)t.tms_stime/clock_ticks);
}
/**
 * 获取进程CPU时间

skydeiMac:10-时间 sky$ ./process_time
CLOCKS_PER_SEC=1000000  sysconf(_SC_CLK_TCK)=100

At program start: 
  clock return: 1974 clocks-per-sec (0.00 secs)
  times yields: user CPU: 0.00, system CPU: 0.00 
numcalls = 10000000
After getppid loop: 
  clock return: 4254428 clocks-per-sec (4.25 secs)
  times yields: user CPU: 3.56, system CPU: 0.69

*/
int
main(int argc, char *argv[])    
{
    int num_calls;
    
    printf("CLOCKS_PER_SEC=%ld  sysconf(_SC_CLK_TCK)=%ld\n\n", (long)CLOCKS_PER_SEC, sysconf(_SC_CLK_TCK));

    display_process_time("At program start: \n");

    num_calls = (argc > 1) ? getInt(argv[1], GN_GT_0, "num-calls") : 10000000;
    printf("numcalls = %d\n", num_calls);

    for (int i=0; i<num_calls; ++i)
        (void)getppid();

    display_process_time("After getppid loop: \n");

    exit(EXIT_SUCCESS);
}

