#define _GNU_SOURCE
#include <sys/wait.h>
#include <sys/utsname.h>
#include <sched.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define STACK_SIZE (1024 * 1024)

static int childFunc(void *arg)
{
    printf("In child process.\n");
    execlp("bash", "bash", (char *) NULL);
    return 0;
}

/*

# echo $$
64267

# ps aux | grep bash | grep -v grep
root     64267  0.0  0.0 115572  2176 pts/0    Ss   16:53   0:00 -bash

# ./a.out           
clone() returned 64360
In child process.

# echo $$
1

# ip addr
1: lo: <LOOPBACK> mtu 65536 qdisc noop state DOWN group default qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00

# exit
exit
child has terminated

# echo $$           
64267

*/
int main(int argc, char *argv[])
{
    char *stack;
    char *stackTop;
    pid_t pid;

    stack = malloc(STACK_SIZE);
    if (stack == NULL)
    {
        perror("malloc"); 
        exit(1);
    }
    stackTop = stack + STACK_SIZE;

    // 指定命名空间的方式创建进程，会进入一个新的 pid、network，以及 mount 的 namespace
    pid = clone(childFunc, stackTop, CLONE_NEWNS|CLONE_NEWPID|CLONE_NEWNET|SIGCHLD, NULL);
    if (pid == -1)
    {
        perror("clone"); 
        exit(1);
    }
    printf("clone() returned %ld\n", (long) pid);

    sleep(1);

    if (waitpid(pid, NULL, 0) == -1)
    {
        perror("waitpid"); 
        exit(1);
    }
    printf("child has terminated\n");
    exit(0);
}
