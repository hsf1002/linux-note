
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
#include "get_num.h"

/**
 *    
非root权限
sleep 60 &
[1] 4327
LD_LIBRARY_PATH=. ./t_setpriority p 4327 -10
setpriority error: Permission denied
Nice value = 0 
LD_LIBRARY_PATH=. ./t_setpriority p 4327 -2
setpriority error: Permission denied
Nice value = 0 
LD_LIBRARY_PATH=. ./t_setpriority p 4327 2
Nice value = 2 
LD_LIBRARY_PATH=. ./t_setpriority p 4327 10
Nice value = 10 

root权限
sudo LD_LIBRARY_PATH=. ./t_setpriority p 4536 -10
Nice value = -10 
sudo LD_LIBRARY_PATH=. ./t_setpriority p 4536 -2
Nice value = -2 
sudo LD_LIBRARY_PATH=. ./t_setpriority p 4536 2
Nice value = 2 
sudo LD_LIBRARY_PATH=. ./t_setpriority p 4536 10
Nice value = 10 

 */
int
main(int argc, char *argv[])    
{
    id_t who;
    int which;
    int prio;

    if (argc != 4 || strchr("pgu", argv[1][0]) == NULL)
        fprintf(stderr, "%s {p|g|u} who priority (p=process, g=group, u=user) \n", argv[0]);
    
    // 进程/进程组/用户ID的
    which = (argv[1][0] == 'p') ? PRIO_PROCESS:
            (argv[1][0] == 'g') ? PRIO_PGRP: PRIO_USER;
    // ID号
    who = getLong(argv[2], 0, "who");
    // 优先级
    prio = getIint(argv[3], 0, "prio");

    // 设置优先级
    if (-1 == setpriority(which, who, prio))
        perror("setpriority error");
    
    errno = 0;
    // 获取优先级
    prio = getpriority(which, who);

    // 优先级可能是0/-1
    if (-1 == prio && 0 != errno)
        perror("getpriority error");
    
    printf("Nice value = %d \n", prio);

    exit(EXIT_SUCCESS);
}

