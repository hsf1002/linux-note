#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/wait.h>


#define MAX_CMD_LEN 200

/**
 * 
 * 打印进程终止的状态
 */
void print_wait_status(const char *msg, int status)
{
    if (NULL != msg)
        printf("%s", msg);
    
    // 正常终止
    if (WIFEXITED(status))
        printf("child existed, status = %d\n", WEXITSTATUS(status));
    // 信号终止
    else if (WIFSIGNALED(status))
    {
        printf("child killed by signal, %d (%s)\n", WTERMSIG(status), strsignal(WTERMSIG(status)));

        #ifdef WCOREDUMP 
            if (WCOREDUMP(status))
                printf("core dumped \n");
        #endif
    }
    // 信号停止
    else if (WIFSTOPPED(status))
    {
        printf("child stopped by signal %d (%s)\n", WSTOPSIG(status), strsignal(WSTOPSIG(status)));
    }
    // 信号恢复
#ifdef WIFCONTINUED
    else if (WIFCONTINUED(status))
        printf("child continued\n");
#endif
    // 不应该走到这里
    else
    {
        printf("what happend to this child? status = %x\n", (unsigned int)status);
    }
}

/**
 *   
 * 循环读取字符串，用system执行它

skydeiMac:27-程序的执行 sky$ cc t_system.c -o t_system
skydeiMac:27-程序的执行 sky$ ./t_system 
command:whoami
sky
system() returned, status = 0x0000 (0, 0)
child existed, status = 0
command:ls
README.md	closeonexec.c	envargs.c	t_execl		t_execle	t_execlp	t_execve	t_system
closeonexec	envargs		necho.script	t_execl.c	t_execle.c	t_execlp.c	t_execve.c	t_system.c
system() returned, status = 0x0000 (0, 0)
child existed, status = 0
command:ls | grep READ
README.md
system() returned, status = 0x0000 (0, 0)
child existed, status = 0
command:exit 127
system() returned, status = 0x7f00 (127, 0)
probably could not invoke shell
command:sleep 30
^Z
[1]+  Stopped                 ./t_system
skydeiMac:27-程序的执行 sky$ ps -a | grep sleep
 1726 ttys001    0:00.00 sleep 30
 1728 ttys001    0:00.00 grep sleep
skydeiMac:27-程序的执行 sky$ kill 1726
skydeiMac:27-程序的执行 sky$ fg
./t_system
system() returned, status = 0x000f (0, 15)
child killed by signal, 15 (Terminated: 15)
*/
int
main(int argc, char *argv[])    
{
    char str[MAX_CMD_LEN];
    int status;

    for (;;)
    {
        printf("command:");
        fflush(stdout);

        if (fgets(str, MAX_CMD_LEN, stdin) == NULL)
            break;
        
        status = system(str);

        printf("system() returned, status = 0x%04x (%d, %d)\n", (unsigned int)status, status >> 8, status & 0xff);

        // 无法调用system
        if (-1 == status)
            perror("system error");
        else
        {
            // 无法正常启动shell
            if (WIFEXITED(status) && WEXITSTATUS(status) == 127)
                printf("probably could not invoke shell\n");
            else
                print_wait_status(NULL, status);
        }
    }
    
    exit(EXIT_SUCCESS);
}

