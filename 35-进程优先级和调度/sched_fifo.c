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
#include <sched.h>


// 每1/4秒打印一次
#define CSEC_STEP 25

/**
 */
static void
use_cpu(char *msg)
{
    struct tms tms;
    // CPU总时长
    int cpu_centisecs;
    // 累计时长25的倍数
    int prev_step = 0;
    // 截止上次运行的时长
    int prev_secs = 0;

    for (;;)
    {
        if (-1 == times(&tms))
            perror("times error");
        
        // CPU运行时间(user+system)，0.x 秒数
        cpu_centisecs = (tms.tms_utime + tms.tms_stime) * 100 / sysconf(_SC_CLK_TCK);

        if (cpu_centisecs >= prev_step + CSEC_STEP)
        {
            prev_step += CSEC_STEP;
            printf("%s (PID: %ld), cpu: %0.2f\n", msg, (long)getpid(), cpu_centisecs/100.00);
        }

        // 运行3秒退出
        if (cpu_centisecs > 300)
            break;
        
        // 每次运行1秒，就让出CPU
        if (cpu_centisecs >= prev_secs + 100)
        {
            prev_secs = cpu_centisecs;
            sched_yield();
        }
    }
}

/**
 *    
sudo ./a.out 
[sudo] hefeng 的密码： 
parent (PID: 11697), cpu: 0.25
parent (PID: 11697), cpu: 0.50
parent (PID: 11697), cpu: 0.75
parent (PID: 11697), cpu: 1.00
child (PID: 11698), cpu: 0.25
child (PID: 11698), cpu: 0.50
child (PID: 11698), cpu: 0.75
child (PID: 11698), cpu: 1.00
parent (PID: 11697), cpu: 1.25
parent (PID: 11697), cpu: 1.50
parent (PID: 11697), cpu: 1.75
parent (PID: 11697), cpu: 2.00
child (PID: 11698), cpu: 1.25
child (PID: 11698), cpu: 1.50
child (PID: 11698), cpu: 1.75
child (PID: 11698), cpu: 2.00
parent (PID: 11697), cpu: 2.25
parent (PID: 11697), cpu: 2.50
parent (PID: 11697), cpu: 2.75
parent (PID: 11697), cpu: 3.00
child (PID: 11698), cpu: 2.25
child (PID: 11698), cpu: 2.50
child (PID: 11698), cpu: 2.75
child (PID: 11698), cpu: 3.00
 */
int
main(int argc, char *argv[])    
{
    struct rlimit limit;
    cpu_set_t set;
    struct sched_param sp;
    
    setbuf(stdout, NULL);
    CPU_ZERO(&set);
    // 只让一个CPU运行
    CPU_SET(1, &set);

    // 设置CPU亲和力
    if (-1 == sched_setaffinity(getpid(), sizeof(cpu_set_t), &set))
        perror("sched_setaffinity error");
    
    // 设置CPU限制，进程运行时长不能超过50s
    limit.rlim_cur = limit.rlim_max = 50;
    if (-1 == setrlimit(RLIMIT_CPU, &limit))
        perror("setrlimit error");
    
    // 获取策略为SCHED_FIFO的最小优先级
    if (-1 == (sp.__sched_priority = sched_get_priority_min(SCHED_FIFO)))
        perror("sched_get_priority_min error");
    // 设置策略和优先级
    if (-1 == sched_setscheduler(0, SCHED_FIFO, &sp))
        perror("sched_setscheduler error");

    // 以最低优先级运行两个进程
    switch (fork())
    {
        case -1:
            perror("fork error");
            exit(EXIT_FAILURE);

        case 0:
            use_cpu("child");
            exit(EXIT_SUCCESS);
        
        default:
            use_cpu("parent");
            exit(EXIT_SUCCESS);
    }
}
