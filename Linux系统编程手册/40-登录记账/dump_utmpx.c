#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <pthread.h>
#include <paths.h>
#include <time.h>
#include <utmpx.h>


/**
 *    显示一个utmpx格式文件的内容
 
 ./a.out 
user       type      PID   line   id   host    data/time
         BOOT_TIME     1                      Sat Dec 28 12:23:39 2019
sky      USER_PROC   100 consol /             Sat Dec 28 18:39:26 2019
sky      USER_PROC   556 ttys00 s000t           Sat Dec 28 18:39:33 2019
sky      USER_PROC   561 ttys00 s001t           Sat Dec 28 18:39:33 2019
sky      USER_PROC   571 ttys00 s002t           Sat Dec 28 18:39:33 2019
sky      USER_PROC   572 ttys00 s003t           Sat Dec 28 18:39:33 2019

./a.out /var/log/wtmp  macOS 显示结果同上
 * 
 */
int
main(int argc, char *argv[])    
{
    struct utmpx *ut;

    if (argc > 1 && 0 == strcmp(argv[1], "--help"))
    {
        fprintf(stderr, "%s [utmpx-pathname]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (argc > 1)
    {
        // 使用备选文件，如果命令行指定的话
        if (-1 == utmpxname(argv[1]))
        {
            perror("utmpxname error");
            exit(EXIT_FAILURE);
        }
    }

    // 将当前位置设置为文件起始位置
    setutxent();

    printf("user       type      PID   line   id   host    data/time\n");

    // 顺序检索文件
    while(NULL != (ut = getutxent()))
    {
        printf("%-8s ", ut->ut_user);
        printf("%-9.9s ", (ut->ut_type == EMPTY) ? "EMPTY" : 
                            (ut->ut_type == RUN_LVL) ? "RUN_LVL" :
                            (ut->ut_type == BOOT_TIME) ? "BOOT_TIME" :
                            (ut->ut_type == NEW_TIME) ? "NEW_TIME" :
                            (ut->ut_type == OLD_TIME) ? "OLD_TIME" :
                            (ut->ut_type == INIT_PROCESS) ? "INIT_PROCESS" :
                            (ut->ut_type == LOGIN_PROCESS) ? "LOGIN_PROCESS" :
                            (ut->ut_type == USER_PROCESS) ? "USER_PROCESS" :
                            (ut->ut_type == DEAD_PROCESS) ? "DEAD_PROCESS" : "???");
        printf("%5ld %-6.6s %-3.5s %-9.9s ", (long)ut->ut_pid, ut->ut_line, ut->ut_id, ut->ut_host);
        printf("%s", ctime((time_t *)&(ut->ut_tv.tv_sec)));
    }
    // 关闭文件
    endutxent();

    exit(EXIT_SUCCESS);
}

