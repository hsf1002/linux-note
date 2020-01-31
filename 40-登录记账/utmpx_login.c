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
#include "get_num.h"


/**
 *    更新utmp/wtmp文件的内容
 su
Password:
sh-3.2# ./utmpx_login sky
creating login entries in utmp and wtmp
    using pid 33003, line ttys001, id s001
^Z
[1]+  Stopped(SIGTSTP)        ./utmpx_login sky
sh-3.2# exit
exit
There are stopped jobs.
sh-3.2# cc -g -Wall -o dump_utmpx dump_utmpx.c 
sh-3.2# ./dump_utmpx
user       type      PID   line   id   host    data/time
         BOOT_TIME     1                      Sat Dec 28 12:23:39 2019
sky      USER_PROC   100 consol /             Sat Dec 28 18:39:26 2019
sky      USER_PROC   556 ttys00 s000t           Sat Dec 28 18:39:33 2019
sky      USER_PROC 33003 ttys00 s001t           Fri Jan 31 20:46:02 2020
sky      USER_PROC   571 ttys00 s002t           Sat Dec 28 18:39:33 2019
sky      USER_PROC   572 ttys00 s003t           Sat Dec 28 18:39:33 2019



fg
./utmpx_login sky
creating logout entries in utmp and wtmp
sh-3.2# ./dump_utmpx
user       type      PID   line   id   host    data/time
         BOOT_TIME     1                      Sat Dec 28 12:23:39 2019
sky      USER_PROC   100 consol /             Sat Dec 28 18:39:26 2019
sky      USER_PROC   556 ttys00 s000t           Sat Dec 28 18:39:33 2019
         DEAD_PROC 33003 ttys00 s001t           Fri Jan 31 20:48:10 2020
sky      USER_PROC   571 ttys00 s002t           Sat Dec 28 18:39:33 2019
sky      USER_PROC   572 ttys00 s003t           Sat Dec 28 18:39:33 2019
 */
int
main(int argc, char *argv[])    
{
    struct utmpx ut;
    char * dev_name;

    if (argc < 2 || 0 == strcmp(argv[1], "--help"))
    {
        fprintf(stderr, "%s username [sleep-time]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    memset(&ut, 0x00, sizeof(struct utmpx));
    // 执行登入信息为用户进程
    ut.ut_type = USER_PROCESS;
    // 命令行指定用户名
    strncpy(ut.ut_user, argv[1], sizeof(ut.ut_user));
    // 设置为当前时间
    if (-1 == time((time_t *)&ut.ut_tv.tv_sec))
        perror("time error");

    ut.ut_pid = getpid();

    // 标准输入相关的终端名
    if (NULL == (dev_name = ttyname(STDIN_FILENO)))
        perror("ttyname error");

    if (strlen(dev_name) <= 8)
        perror("terminal name is too short");

    strncpy(ut.ut_line, dev_name + 5, sizeof(ut.ut_line));
    strncpy(ut.ut_id, dev_name + 8, sizeof(ut.ut_id));

    printf("creating login entries in utmp and wtmp\n");
    printf("    using pid %ld, line %.*s, id %.*s\n", (long)ut.ut_pid, (int)sizeof(ut.ut_line), ut.ut_line, (int)sizeof(ut.ut_id), ut.ut_id);

    // 将当前位置设置为文件起始位置
    setutxent();

    // 写一条记录到utmp文件
    if (NULL == pututxline(&ut))
        perror("pututxline error");
    // 写一条记录追加到wtmp文件
    //updwtmpx(_PATH_WTMP, &ut);

    // 睡眠一段时间后登出
    sleep((argc > 2) ? getInt(argv[2], GN_NONNEG, "sleep-time") : 15);

    // 指定登出信息为已停止进程
    ut.ut_type = DEAD_PROCESS;
    // 设置为当前时间
    time((time_t *)&ut.ut_tv.tv_sec);
    // 指定登出名为空
    memset(&ut.ut_user, 0x00, sizeof(ut.ut_user));

    printf("creating logout entries in utmp and wtmp\n");
    // 将当前位置设置为文件起始位置
    setutxent();

    // 覆盖之前的记录
    if (NULL == pututxline(&ut))
        perror("pututxline error");
    // 写一条记录追加到wtmp文件
    //updwtmpx(_PATH_WTMP, &ut);

    // 关闭文件
    endutxent();

    exit(EXIT_SUCCESS);
}

