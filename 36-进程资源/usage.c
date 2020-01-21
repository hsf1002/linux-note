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

/**
 *    
 LD_LIBRARY_PATH=. ./usage thread_lock_speed
child (PID=29697) started

	 CPU time (sec):         user=0.000, system=0.000
	 max resident set size:  728 
	 int shared memory:      0 
	 int unshared data:      0 
	 int unshared stack:     0 
	 page reclaims:          18 
	 page faults:            0 
	 swaps:                  0 
	 block I/O:              input=0, output = 0
	 signals received:       0 
	 IPC messages:           sent=0, received=0
	 context switches:       voluntary=1, involuntary=0 

*/
int
main(int argc, char *argv[])    
{
    struct rusage usage;
    pid_t pid;

    if (argc < 2 || 0 == strcmp(argv[1], "--help"))
        printf(stderr, "%s command arg... \n", argv[0]);
    
    switch (pid = fork())
    {
        case -1:
            perror("fork error");
            exit(EXIT_FAILURE);

        case 0:
            execvp(argv[1], &argv[1]);
            _exit(EXIT_SUCCESS);
        
        default:
            printf("child (PID=%ld) started\n", (long)pid);

            if (-1 == wait(NULL))
                perror("wait error");
            if (-1 == getrusage(RUSAGE_CHILDREN, &usage))
                perror("get_usage error");
            
            printf("\n");
            print_usage("\t", &usage);
            exit(EXIT_SUCCESS);
    }
}
