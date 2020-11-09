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
#include <sys/types.h>
#include <sched.h>
#include <signal.h>
#include <time.h>
#include <syslog.h>
#include <getopt.h>


static void
usage_error(const char *prog_name)
{
    fprintf(stderr, "usage: %s [-p][-e][-l] message \n", prog_name);
    fprintf(stderr, "    -p log PID\n");
    fprintf(stderr, "    -e log to stderr also \n");
    fprintf(stderr, "    -l level (g=EMERG, a=ALERT, c=CRIT, e=ERR, w=WARNNING, n=NOTICE, i=INFO, d=DEBUG)\n");
    exit(EXIT_FAILURE);
}

/**
 * 
./a.out p
/var/log/syslog

Jan 17 15:49:18 sw-hefeng ./a.out: hello world start
Jan 17 15:49:18 sw-hefeng ./a.out: p
Jan 17 15:49:18 sw-hefeng ./a.out: hello world end

 */
int main(int argc, char *argv[])
{
    int level = LOG_INFO;
    int options = 0;
    int opt;

    // l必须带参数
    while ((opt = getopt(argc, argv, "l:pe")) != -1)
    {
        switch (opt)
        {
            case 'l':
            {
                switch (optarg[0])
                {
                    case 'a':
                        level = LOG_ALERT;
                    break;
                    case 'c':
                        level = LOG_CRIT;
                    break;
                    case 'e':
                        level = LOG_ERR;
                    break;
                    case 'w':
                        level = LOG_WARNING;
                    break;
                    case 'n':
                        level = LOG_NOTICE;
                    break;
                    case 'i':
                        level = LOG_INFO;
                    break;
                    case 'd':
                        level = LOG_DEBUG;
                    break;
                    default:
                        fprintf(stderr, "bad facility: %c \n", optarg[0]);
                    break;
                }
            }
            break;
            case 'p':
                options |= LOG_PID;
            break;
        #if !defined(__hpux) && !defined(__sun)
            case 'e':
                options |= LOG_PERROR;
            break;
        #endif
            default:
                fprintf(stderr, "bad option\n");
                usage_error(argv[0]);
            break;
        }
    }

    if (argc != optind + 1)
        usage_error(argv[0]);
    
    // 建立进程与syslog之间的连接
    openlog(argv[0], options, LOG_USER);
    // 开始
    syslog(LOG_INFO, "hello world start\n");
    // argv[optind]是最后一个参数
    syslog(LOG_USER | level, "%s", argv[optind]);
    syslog(LOG_INFO, "hello world end\n");
    // 断开连接
    closelog();

    exit(EXIT_SUCCESS);
}
