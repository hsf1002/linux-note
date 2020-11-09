#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <time.h>
#include "get_num.h"


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

/**
 *   
 * 通过waitpid获取子进程状态

cc -g -Wall -o child_status child_status.c libgetnum.so libwaitstatus.so

skydeiMac:26-监控子进程 sky$ ./child_status 23
11:17:14 child started with PID = 69413
waitpid() returned PID = 69413, status = 0x1700, (23, 0)
child existed, status = 23
skydeiMac:26-监控子进程 sky$ ./child_status &
[1] 69414
skydeiMac:26-监控子进程 sky$ 11:17:19 child started with PID = 69415

skydeiMac:26-监控子进程 sky$ kill -STOP 69415
waitpid() returned PID = 69415, status = 0x117f, (17, 127)
child stopped by signal 17 (Suspended (signal): 17)
skydeiMac:26-监控子进程 sky$ kill -CONT 69415
skydeiMac:26-监控子进程 sky$ ps -a | grep 69415
69415 ttys002    0:00.00 ./child_status
69419 ttys002    0:00.00 grep 69415
skydeiMac:26-监控子进程 sky$ kill -ABRT 69415

skydeiMac:26-监控子进程 sky$ waitpid() returned PID = 69415, status = 0x0086, (0, 134)
child killed by signal, 6 (Abort trap: 6)
core dumped
u
-bash: u: command not found
[1]+  Done                    ./child_status
 */
int
main(int argc, char *argv[])    
{
    int status;
    pid_t child_pid;

    if (argc > 1 && 0 == strcmp(argv[1], "--help"))
        fprintf(stderr, "%s [exit_status]\n", argv[0]);

    switch (fork())
    {
        case -1:
            perror("fork error");
        break;
        case 0:
            printf("%s child started with PID = %ld \n", curr_time("%T"), (long)getpid());
            
            // 正常终止
            if (argc > 1)
                exit(getInt(argv[1], 0, "exit_status"));
            // 等待信号终止
            else
                for (;;)
                    pause();
            // 不会走到这里来，但是最好加上
            exit(EXIT_FAILURE);
        //break;
        default:
        {
            for (;;)
            {
                // 若子进程正常终止或因信号而停止或恢复，都将返回状态
                if (-1 == (child_pid = waitpid(-1, &status, WUNTRACED
            #ifdef WCONTINUED
                                                          | WCONTINUED
            #endif    
                )))
                    perror("waitpid error\n");

                printf("waitpid() returned PID = %ld, status = 0x%04x, (%d, %d)\n", (long)child_pid, (unsigned int)status, status >> 8, status & 0xff);

                // 打印子进程退出的状态
                print_wait_status(NULL, status);

                // 只有正常终止或因信号终止时，父进程才退出
                if (WIFEXITED(status) || WIFSIGNALED(status))
                    exit(EXIT_SUCCESS);
            }
        }
        //break;
    }
}

