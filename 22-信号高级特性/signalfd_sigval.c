#define _GNU_SOURCE
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
#include <sys/signalfd.h>



/**
 *   
 * sigwaitinfo的替代方案
 * 
 * 
./t_signalfd 44 &
./sigqueue pid 44 123




 */
int
main(int argc, char *argv[])    
{
    struct sigaction sa;
    siginfo_t si;
    sigset_t block_mask;
    int sig_no;
    struct signalfd_siginfo fdsi;
    ssize_t s;
    int fd;

    if (argc < 2 || 0 == strcmp(argv[1], "--help"))
        fprintf(stderr, "%s sig-no\n", argv[0]);

    printf("%s: pid is %ld\n", argv[0], (long)getpid());

    sigemptyset(&block_mask);

    for (int i=1; i<argc; i++)
        sigaddset(&block_mask, atoi(argv[i]));

    // 阻塞通过参数指定的信号，除了SIGKILL和SIGSTOP
    if (-1 == sigprocmask(SIG_SETMASK, &block_mask, NULL))
        perror("sigprocmask error");

    if (-1 == (fd = signalfd(-1, &mask, 0)))
        perror("signalfd error");

    for (;;)
    {
        // 从文件描述符读取信号
        if (sizeof(struct signalfd_siginfo) != (s = read(fd, &fdsi, sizeof(struct signalfd_siginfo))))
            perror("read error");

        // 否则打印出接收到的信号信息
        printf("got signal: %d (%s)\n", sig_no, strsignal(sig_no));

        if (SI_QUEUE == fdsi.ssi_code)
        {
            printf("  ,si_pid = %ldn", (long)fdsi.ssi_pid);
            printf("  ,ssi_int = %dn", fdsi.ssi_int);
        }

        printf("\n");
    }

    exit(EXIT_SUCCESS);
}

