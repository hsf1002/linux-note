#define _GNU_SOURCE
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
#include <sys/times.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sched.h>
#include <signal.h>
#include <time.h>
#include <stdarg.h>
#include "become_daemon.h"

#define TS_BUF_SIZE sizeof("YYYY-MM-DD HH:MM:SS")    
#define SBUF_SIZE 100


static const char *LOG_FILE = "/tmp/ds.log";
static const char *CONFIG_FILE = "/tmp/ds.conf";

static FILE *logfp;

static volatile sig_atomic_t hup_received = 0;

/**
 * 
 * 向文件中写入内容
 */
static void
log_msg(const char *format, ...)
{
    va_list arglist;
    const char *TIMESTAMP_FMT = "%F %X"; // YYYY-MM-DD HH:MM:SS
    char timestamp[TS_BUF_SIZE];
    time_t t;
    struct tm *loc;

    t = time(NULL);
    if (NULL == (loc = localtime(&t)) || strftime(timestamp, TS_BUF_SIZE, TIMESTAMP_FMT, loc) == 0)
        fprintf(logfp, "???? unknown time????");
    else
        fprintf(logfp, "%s: ", timestamp);

    va_start(arglist, format);
    vfprintf(logfp, format, arglist);
    fprintf(logfp, "\n");
    va_end(arglist);
}

/**
 * 打开文件
 */ 
static void
log_open(const char *file_name)
{
    mode_t m;

    m = umask(077);
    logfp = fopen(file_name, "a");
    umask(m);

    if (NULL == logfp)
        exit(EXIT_FAILURE);
    
    setbuf(logfp, NULL);

    log_msg("opened log file");
}

/**
 * 关闭文件
 */ 
static void 
log_close(void)
{
    log_msg("close log file");
    fclose(logfp);
}

/**
 * 读取配置信息并写入文件
 */ 
static void
read_cfg_file(const char *file_name)
{
    FILE *fp;
    char str[SBUF_SIZE];

    if (NULL == (fp = fopen(file_name, "r")))
        exit(EXIT_FAILURE);
    
    if (NULL == fgets(str, SBUF_SIZE, fp))
        str[0] = '\0';
    else
        str[strlen(str) - 1] = '\0';
    
    log_msg("read cfg file: %s", str);
    fclose(fp);
}

/**
 * 信号处理函数
 */ 
static void
sig_handler(int sig)
{
    hup_received = 1;
    printf("got signal sig = %d \n", sig);
}

/**
 *    
 */
int
main(int argc, char *argv[])    
{
    const int SLEEP_TIME = 15;
    int count = 0;
    int unslept;
    struct sigaction sa;

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sa.sa_handler = sig_handler;

    // 为SIGHUP注册信号处理函数
    if (-1 == sigaction(SIGHUP, &sa, NULL))
        perror("sigaction error");
    
    // 变成守护进程
    if (-1 == become_daemon(0))
        perror("become_daemon error");
    
    // 打开日志
    log_open(LOG_FILE);
    // 读取配置
    read_cfg_file(CONFIG_FILE);

    unslept = SLEEP_TIME;

    for (;;)
    {
        // 睡眠15秒
        unslept = sleep(unslept);

        // 如果收到信号（大概率睡眠被中断，由于设置了SA_RESTART ??? 继续睡眠？）
        if (hup_received)
        {
            hup_received = 0;
            log_close();
            log_open(LOG_FILE);
            read_cfg_file(CONFIG_FILE);
        }

        // 如果睡眠正常结束
        if (0 == unslept)
        {
            count++;
            log_msg("main: %d", count);
            unslept = SLEEP_TIME;
        }
    }
    
    exit(EXIT_SUCCESS);
}
