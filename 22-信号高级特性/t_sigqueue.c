#define _POSIX_C_SOURCE 199309
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
#include <setjmp.h>
#include "get_num.h"


/**
 *   
 * 使用sigqueue发送实时信号
 * 
 

 */
int
main(int argc, char *argv[])    
{
    int sig_no;
    int num_sig;
    int sig_data;
    union sigval sv;

    if (argc < 4 || 0 == strcmp(argv[1], "--help"))
        fprintf(stderr, "%s pid sig_no data [num-sigs]\n", argv[0]);
    
    printf("%s PID: %ld, UID: %ld\n ", argv[0], (long)getpid(), (long)getuid());

    sig_no = getInt(argv[2], 0, "sig-num");
    sig_data = getInt(argv[3], GN_ANY_BASE, "data");
    num_sig = (argc > 4) ? getInt(argv[4], GN_GT_0, "num_sig") : 1;

    for (int i=0; i<num_sig; i++)
    {
        sv.sival_int = sig_data + 1;
        if (-1 == sigqueue(getLong(argv[1], 0, "pid"), sig_no, sv))
            perror("sigqueue error");
    }

    exit(EXIT_SUCCESS);
}

