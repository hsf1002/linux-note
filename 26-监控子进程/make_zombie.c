#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <libgen.h>


#define CMD_SIZE 200

/**
 *   
 * 创建一个僵尸进程，发送SIGKILL无法杀死它
cc -g -Wall -o make_zombie make_zombie.c

./make_zombie
parent PID = 18659
child with PID = 69794 
69793 ttys002    0:00.00 ./make_zombie
69794 ttys002    0:00.00 (make_zombie)
69795 ttys002    0:00.01 sh -c ps -a | grep make_zombie
69797 ttys002    0:00.00 grep make_zombie
after sending SIGKILL signal to zombie(PID = 69794)
69793 ttys002    0:00.01 ./make_zombie
69794 ttys002    0:00.00 (make_zombie)
69798 ttys002    0:00.00 sh -c ps -a | grep make_zombie
69800 ttys002    0:00.00 grep make_zombie
 */
int
main(int argc, char *argv[])    
{
    char cmd[CMD_SIZE];
    pid_t child_pid;

    setbuf(stdout, NULL);

    printf("parent PID = %ld\n", (long)getppid());

    switch (child_pid = fork())
    {
        case -1:
            perror("fork error");
        break;
        case 0:
            printf("child with PID = %ld \n", (long)getpid());

            _exit(EXIT_FAILURE);
        //break;
        default:
        {
            // 让子进程创建并终止，但是没有调用wait
            sleep(3);

            snprintf(cmd, CMD_SIZE, "ps -a | grep %s", basename(argv[0]));
            cmd[CMD_SIZE - 1] = '\0';
            // 查看进程是否存在：存在
            system(cmd);
            // 发送SIGKILL给僵尸进程
            if (-1 == kill(child_pid, SIGKILL))
                perror("kill error");
            
            sleep(3);

            printf("after sending SIGKILL signal to zombie(PID = %ld)\n", (long)child_pid);
            // 僵尸进程依然存在
            system(cmd);

            exit(EXIT_SUCCESS);
        }
        //break;
    }
}

