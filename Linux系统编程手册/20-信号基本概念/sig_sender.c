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
#include "get_num.h"


/**
 *   
 * 发送多个信号
 */
int
main(int argc, char *argv[])    
{
    int sig_cnt;
    int sig_no;
    pid_t pid;

    if (argc < 4 || 0 == strcmp(argv[1], "--help"))
        fprintf(stderr, "%s pid num-sigs sig-num [sig-num-2]\n", argv[0]);
    
    pid = getLong(argv[1], 0, "pid");
    sig_cnt = getInt(argv[2], GN_GT_0, "sig-cnt");
    sig_no = getInt(argv[3], 0, "sig-no");

    printf("%s: sending signal %d to process %ld %d times\n", argv[0], sig_no, (long)pid, sig_cnt);

    // 连续多次发送同一个信号
    for (int i=0; i<sig_cnt; i++)
        if (-1 == kill(pid, sig_no))
            perror("kill error");

    // 仅仅发送一次不同的信号
    if (argv > 4)
        if (-1 == kill(pid, getInt(argv[4], 0, "sig-num-2")))
            perror("kill error");

    printf("%s: existing\n", argv[0]);

    exit(EXIT_SUCCESS);
}

