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

##### 系统调用：clone

Linux特有的系统调用clone也能创建一个新进程，与fork和vfork相比，在进程创建期间对步骤的控制更为精准，clone主要用于线程库的实现

```
#define _GNU_SOURCE
#include <sched.h>

int clone(int (*fn)(void *), void *stack, int flags, void *arg, ...
  /* pid_t *parent_tid, void *tls, pid_t *child_tid */ );
// 返回值：若成功，返回子进程PID，若出错，返回-1  
// 对于内核而言，fork、vfork和clone由同一系统调用do_fork实现
// 当函数fn返回或是调用exit/_exit时，子进程就会终止
```

flag的双重目的：低字节存放子进程的终止信号，因信号而终止依然产生并给父进程发送SIGCHLD信号，如果是0表示没有产生任何信号；剩余字节存放了位掩码：

```
CLONE_CHILD_CLEARTID: 当子进程调用exec或exit时，清除ctid
CLONE_CHILD_SETTID: 将子进程的线程ID写入ctid
CLONE_PARENT_SETTID：将子进程的线程ID写入ptid
CLONE_FILES: 父子进程共享打开文件描述符
CLONE_FS: 父子进程共享与文件系统相关的属性，涉及的系统调用umask、chdir、chroot
CLONE_IO: 父子进程共享IO上下文环境
CLONE_VM: 父子进程共享虚拟内存，涉及的系统调用mmap、munmap
CLONE_SIGHAND: 父子进程共享对信号的处置
CLONE_SETTLS: tls描述子进程的线程本地存储
CLONE_NEWNS: 子进程获取父进程挂载命名空间的副本
CLONE_PARENT: 子进程.PPID == 调用者.PPID（默认子进程的PPID == 调用者.PID）
CLONE_THREAD: 将子进程置于父进程所属的线程组中（一个线程组每个线程都有一个唯一的TID，所有线程的TGID相同，第一个线程的TID即该线程组的TGID，以后每个线程的TID依次递加）
CLONE_VFORK: 父进程一直挂起，直到子进程调用exec或exit释放虚拟内存资源
...
```

fork相当于如下flag的clone：

```
SIGCHLD
```

vfork相当于如下flag的clone：

```
CLONE_VM |CLONE_VFORK | CLONE_SIGCHLD 
```

LinuxThreads线程相当于如下flag的clone：

```
CLONE_VM | CLONE_FILES | CLONE_FS | CLONE_SIGCHLD 
```

NPTL线程相当于如下flag的clone：

```
CLONE_VM | CLONE_FILES | CLONE_FS | CLONE_SIGCHLD | CLONE_THREAD | CLONE_SETTLS | CLONE_PARENT_SETTID | CLONE_CHLD_CLEARTID | CLONE_SYSVSEM
```

等待由clone产生的子进程，waitpid、wait3、wait4的位掩码取值：

* __WCLONE：只等待克隆子进程，如未设置，只等待非克隆子进程
* __WALL：等待所有子进程，不论类型
* __WNOTHREAD：调用者只等待自己的子进程，如未设置，等待与父进程隶属同一线程组的任何进程

##### 进程的创建速度

* fork：进程所占内存越大，耗时越久
* vfork：进程所占内存大小，影响不大，因为并未像fork一样复制页表
* clone：进程所占内存大小，影响不大，速度最快

##### exec和fork对进程属性的影响

进程有很多属性，这存在两个问题：

1. 进程执行exec时，进程属性如何变化，哪些属性会得以保存？
2. 进程执行fork时，进程属性如何变化，哪些属性会得到继承？

进程属性主要分为：进程地址空间、进程标识符和凭证、文件IO和目录、文件系统、信号、定时器、POSIX线程、优先级和调度、资源与CPU时间、进程间通信、Linux特有的诸多属性

