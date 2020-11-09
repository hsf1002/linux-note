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
// 设置5个阻塞信号
hefeng@sw-hefeng:/home/workspace1/logs/test$ LD_LIBRARY_PATH=. ./signalfd_sigval 40 41 42 43 44 &
[1] 20820
hefeng@sw-hefeng:/home/workspace1/logs/test$ ./signalfd_sigval: pid is 20820

// 发送一个实时信号
 hefeng@sw-hefeng:/home/workspace1/logs/test$ LD_LIBRARY_PATH=. ./sigqueue 20820 44 400 1
./sigqueue PID: 20826, UID: 1000
 got signal: 5076612 (Unknown signal 5076612)
  ,si_pid = 20826n  ,ssi_int = 401n
// 发送一个实时信号
hefeng@sw-hefeng:/home/workspace1/logs/test$ LD_LIBRARY_PATH=. ./sigqueue 20820 43 300 1
./sigqueue PID: 20827, UID: 1000
 got signal: 5076612 (Unknown signal 5076612)
  ,si_pid = 20827n  ,ssi_int = 301n
// 发送一个实时信号
hefeng@sw-hefeng:/home/workspace1/logs/test$ LD_LIBRARY_PATH=. ./sigqueue 20820 42 200 1
./sigqueue PID: 20829, UID: 1000
 got signal: 5076612 (Unknown signal 5076612)
  ,si_pid = 20829n  ,ssi_int = 201n
// 发送一个实时信号
hefeng@sw-hefeng:/home/workspace1/logs/test$ LD_LIBRARY_PATH=. ./sigqueue 20820 41 100 1
./sigqueue PID: 20830, UID: 1000
 got signal: 5076612 (Unknown signal 5076612)
  ,si_pid = 20830n  ,ssi_int = 101n
// 发送一个实时信号
hefeng@sw-hefeng:/home/workspace1/logs/test$ LD_LIBRARY_PATH=. ./sigqueue 20820 40 100 1
./sigqueue PID: 20831, UID: 1000
 got signal: 5076612 (Unknown signal 5076612)
  ,si_pid = 20831n  ,ssi_int = 101n
// 发送一个实时信号
hefeng@sw-hefeng:/home/workspace1/logs/test$ LD_LIBRARY_PATH=. ./sigqueue 20820 41 101 1
./sigqueue PID: 20832, UID: 1000
 got signal: 5076612 (Unknown signal 5076612)
  ,si_pid = 20832n  ,ssi_int = 102n
// 发送一个实时信号
hefeng@sw-hefeng:/home/workspace1/logs/test$ LD_LIBRARY_PATH=. ./sigqueue 20820 42 102 1
./sigqueue PID: 20833, UID: 1000
 got signal: 5076612 (Unknown signal 5076612)
  ,si_pid = 20833n  ,ssi_int = 103n
// 发送一个非阻塞的实时信号，终止了进程
hefeng@sw-hefeng:/home/workspace1/logs/test$ LD_LIBRARY_PATH=. ./sigqueue 20820 45 105 1
./sigqueue PID: 20836, UID: 1000
 [1]+  实时信号 11         LD_LIBRARY_PATH=. ./signalfd_sigval 40 41 42 43 44
hefeng@sw-hefeng:/home/workspace1/logs/test$ ps|grep signal




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

    if (-1 == (fd = signalfd(-1, &block_mask, 0)))
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

