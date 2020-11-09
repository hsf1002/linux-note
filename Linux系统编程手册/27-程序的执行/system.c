#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>

/*

*/
int my_system(char *command)
{
    sigset_t block_mask;
    sigset_t orig_mask;
    struct sigaction sa_ignore, sa_origout, sa_origin, sa_default;
    pid_t child_pid;
    int status;
    int saved_errno;

    // 测试shell是否可用
    if (NULL == command)
        return system(":") == 0;
    
    // 阻塞SIGCHLD信号
    sigemptyset(&block_mask);
    sigaddset(&block_mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &block_mask, &orig_mask);

    // 忽略信号SIGINT和SIGQUIT
    sa_ignore.sa_handler = SIG_IGN;
    sa_ignore.sa_flags = 0;
    sigemptyset(&sa_ignore.sa_mask);
    sigaction(SIGINT, &sa_ignore, &sa_origin);
    sigaction(SIGQUIT, &sa_ignore, &sa_origout);

    printf("parent: sa_origin.sa_handler = %d\n", sa_origin.sa_handler);
    printf("parent: sa_origout.sa_handler = %d\n", sa_origout.sa_handler);

    switch (child_pid = fork())
    {
        case -1:
            perror("fork error");
            status = -1;
        break;
        // 子进程
        case 0:
            // 恢复父进程对SIGINT和SIGQUIT的变更，修改为默认处置
            sa_default.sa_handler = SIG_DFL;
            sa_default.sa_flags = 0;
            sigemptyset(&sa_default.sa_mask);

            // 从父进程继承，sa_origin.sa_handler应该是SIG_DFL
            printf("child: sa_origin.sa_handler = %d\n", sa_origin.sa_handler);
            printf("child: sa_origout.sa_handler = %d\n", sa_origout.sa_handler);

            if (SIG_IGN != sa_origin.sa_handler)
                sigaction(SIGINT, &sa_default, NULL);
            if (SIG_IGN != sa_origout.sa_handler)
                sigaction(SIGQUIT, &sa_default, NULL);

            // 解除对信号SIGCHLD的阻塞
            sigprocmask(SIG_SETMASK, &orig_mask, NULL);

            execl("/bin/sh", "sh", "-c", command, (char *)NULL);
            _exit(127);
        //break;
        // 父进程
        default:
            while (waitpid(child_pid, &status, 0) == -1)
            {
                if (errno != EINTR)
                {
                    status = -1;
                    break;
                }
            }
        break;
    }

    // 子进程退出，只有父进程会跑进来
    saved_errno = errno;

    // 主程序解除对SIGCHLD的阻塞
    sigprocmask(SIG_SETMASK, &orig_mask, NULL);
    // 主程序恢复对SIGINT和SIGQUIT的处置
    sigaction(SIGINT, &sa_origin, NULL);
    sigaction(SIGQUIT, &sa_origout, NULL);

    errno = saved_errno;

    return status;
}

/**
 *   
 * 考虑到信号处理的system实现

./a.out 
parent: sa_origin.sa_handler = 0
parent: sa_origout.sa_handler = 0
child: sa_origin.sa_handler = 0
child: sa_origout.sa_handler = 0
Darwin
Segmentation fault: 11 (core dumped)
*/
int
main(int argc, char *argv[])    
{
    printf(my_system("uname"));
    
    exit(EXIT_SUCCESS);
}

