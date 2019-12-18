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
#include <sys/wait.h>
#include <signal.h>
#include <time.h>


#define SYNC_SIG SIGUSR1

/**
 * 
 *  按照格式化要求显示时间格式
 */
char *
curr_time(const char *format)
{
    static char buf[BUFSIZ];  
    time_t t;
    size_t s;
    struct tm *tm;

    t = time(NULL);
    tm = localtime(&t);

    if (tm == NULL)
        return NULL;

    s = strftime(buf, BUFSIZ, (format != NULL) ? format : "%c", tm);

    return (s == 0) ? NULL : buf;
}

static void
handler(int signo)
{
    printf("caught signal: %d\n", signo);
}

/**
 *   
 * 以信号方式同步父子进程，假设父进程要等待子进程完成再继续执行
1. 若父进程试图在fork之后阻塞该信号，则竞争条件将出现
2. 此程序假设与子进程的信号屏蔽字状态无关，如有必要，可以在fork之后的子进程中解除对此信号的阻塞


parent-time: 06:53:14, 65015, prepare to receive signal
child-time: 06:53:14, 65016
child-time: 06:53:16, 65016, prepare to send signal
caught signal: 30
sigsuspend error: Interrupted system call
parent-time: 06:53:16, 65015, received signal
parent-time: 06:53:16, 65015, restored the mask
 */
int
main(int argc, char *argv[])    
{
    pid_t c_pid;
    sigset_t block_mask;
    sigset_t orig_mask;
    sigset_t empty_mask;
    struct sigaction sa;
    int num_child = 10;
    
    setbuf(stdout, NULL);

    // 阻塞信号SYNC_SIG
    sigemptyset(&block_mask);
    sigaddset(&block_mask, SYNC_SIG);
    if (-1 == sigprocmask(SIG_BLOCK, &block_mask, &orig_mask))
        perror("sigprocmask error");
    
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sa.sa_handler = handler;
    // 注册信号处理函数
    if (-1 == sigaction(SYNC_SIG, &sa, NULL))
        perror("sigaction error");

    //for (int i=0; i<num_child; i++)
    {
        switch (c_pid = fork())
        {
        // error
        case -1:
            perror("fork error");
            break;
        // 子进程
        case 0:
            printf("child-time: %s, %ld\n", curr_time("%T"), (long)getpid());

            // 模拟耗时操作
            sleep(2);

            printf("child-time: %s, %ld, prepare to send signal\n", curr_time("%T"), (long)getpid());
            
            // 给父进程发送信号，发送完毕就退出
            if (-1 == kill(getppid(), SYNC_SIG))
                perror("kill error");

            _exit(EXIT_SUCCESS);
        // 父进程
        default:
            
            printf("parent-time: %s, %ld, prepare to receive signal\n", curr_time("%T"), (long)getpid());
            
            sigemptyset(&empty_mask);
            // 父进程阻塞在此，等待信号中断
            if (-1 == sigsuspend(&empty_mask) && EINTR == errno)
                perror("sigsuspend error");

            printf("parent-time: %s, %ld, received signal\n", curr_time("%T"), (long)getpid());

            // 恢复信号屏蔽字
            if (-1 == sigprocmask(SIG_SETMASK, &orig_mask, NULL))
                perror("sigprocmask error");

            printf("parent-time: %s, %ld, restored the mask\n", curr_time("%T"), (long)getpid());

            exit(EXIT_SUCCESS);
        }
    }
}

