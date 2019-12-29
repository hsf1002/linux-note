### 第28章 详述进程创建和程序执行

##### 进程记账

打开进程记账功能后，内核会在每个进程终止时将一条记账信号写入系统级的记账文件，包括终止状态进程消耗的CPU时间，只有当最后一个线程退出才会为整个进程保存一条账单记录，如果进程的信息并未由父进程进行监控和报告，就可以使用记账方式来获取，Linux中记账功能属于可选组件，需要CONFIG_BSD_PROCESS_ACCT配置

```
#define _BSD_SOURCE
#include <unistd.h>

int acct(const char *acctfile);
// 返回值：若成功，返回0，若出错，返回-1
// 只有特权进程才能调用，应用程序很少使用
// 打开：acctfile指定为一个常规文件的路径名，通常是/var/log/pacct或/usr/account/pacct；关闭，acctfile指定为为NULL
// 如果系统奔溃，不会记录任何记账信息
```

记账记录的结构：

```
struct acct
{
    char     ac_flag;       /* flag */                   
    uid_t    ac_uid;        /* real user ID */
    gid_t    ac_gid;        /* real group ID */
    dev_t    ac_tty;        /* controlling terminal */
    
    time_t   ac_btime;      /* starting calendar time */
    comp_t   ac_utime;      /* user CPU time (clock ticks) */
    comp_t   ac_stime;      /* system CPU time (clock ticks) */
    comp_t   ac_etime;      /* elapsed time (clock ticks) */
    
    comp_t   ac_mem;        /* average memory usage */
    comp_t   ac_io;         /* bytes transferred (by read and write) */
    comp_t   ac_rw;         /* blocks read or written */
    char     ac_comm[8];    /* command name: [8] for Solaris, */
    u_int32_t ac_exitcode;  /* process termination status */
};

ac_flag的取值：
AFORK: 由fork创建的进程，终止前并未调用exec
ASU:   拥有超级用户特权的进程
AXSIG: 进程因信号而停止
ACORE: 进程产生了核心转储文件
```

Linux提供了/proc/sys/kernel/acct，包含三个值，按顺序是高水位、低水位、频率，默认值通常是4、2、30，磁盘空间百分比大于高水位开启记账，小于低水位停止记账，频率是两次检查的时间间隔秒数

进程记账文件格式（版本3）

---

从内核2.6.8开始引入，配置选项是CONFIG_BSD_PROCESS_ACCT_V3，与传统acct结构的主要差别是：

1. 增加ac_version：账单记录的版本号
2. 增加ac_pid和ac_ppid：终止进程的进程ID及其父进程ID
3. ac_uid和ac_gid从16位扩展到32位
4. 将ac_etime从comp_t改为float，意在能够记录更长的时间