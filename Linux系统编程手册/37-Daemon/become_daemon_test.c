//#define _GNU_SOURCE
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
#include <sys/stat.h>
#include <sched.h>
#include <signal.h>
#include <time.h>
#include "become_daemon.h"


/**

LD_LIBRARY_PATH=. ./become_daemon_test 
pid=865
ps -aux | grep become_daemon_test
hefeng     865  0.0  0.0   6412    88 pts/2    S    14:55   0:00 ./become_daemon_test
hefeng     867  0.0  0.0  15984   928 pts/2    S+   14:55   0:00 grep --color=auto become_daemon_test
ps -aux | grep become_daemon_test
hefeng     870  0.0  0.0  15984   944 pts/2 
 *    
 */
int
main(int argc, char *argv[])    
{
    // 将本程序变成守护进程
    become_daemon(0);

    // 通常守护程序不会终止，此处只运行8秒
    sleep(8);

    exit(EXIT_SUCCESS);
}
