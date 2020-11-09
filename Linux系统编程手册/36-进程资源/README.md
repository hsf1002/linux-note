## 第36章 进程资源

### 进程资源使用

getrusage返回调用进程极其子进程用掉的各类系统资源的统计信息：

```
#include <sys/resource.h>

int getrusage(int who, struct rusage *res_usage);
// 若成功，返回0，若出错，返回-1

who的取值：
RUSAGE_SELF：返回调用进程的信息
RUSAGE_CHILDREN：返回调用进程的所有被终止和处于等待状态的子进程的信息
RUSAGE_THREAD：返回调用线程的信息

struct rusage 
{
    struct timeval ru_utime; /* user time used 用户态使用的时间 */
    struct timeval ru_stime; /* system time used 内核态使用的时间 */
    long   ru_maxrss;        /* maximum resident set size  */
    long   ru_ixrss;         /* integral shared memory size */
    long   ru_idrss;         /* integral unshared data size */
    long   ru_isrss;         /* integral unshared stack size */
    long   ru_minflt;        /* page reclaims */
    long   ru_majflt;        /* page faults */
    long   ru_nswap;         /* swaps */
    long   ru_inblock;       /* block input operations */
    long   ru_oublock;       /* block output operations */
    long   ru_msgsnd;        /* messages sent */
    long   ru_msgrcv;        /* messages received */
    long   ru_nsignals;      /* signals received */
    long   ru_nvcsw;         /* voluntary context switches */
    long   ru_nivcsw;        /* involuntary context switches */
};
```

### 进程资源限制

Linux特有的/proc/PID/limits文件可以查看任意进程的所有资源限制

```
#include <sys/resource.h>

int getrlimit(int resource, struct rlimit *rlim);
int setrlimit(int resource, const struct rlimit *rlim);
// 若成功，返回0，若出错，返回-1

struct rlimit
{
    rlim_t rlim_cur;  // 软限制：实际限制（取值0到硬限制之间）
    rlim_t rlim_max;  // 硬限制：上限（特权CAP_SYS_RESOUCE进程能够增减硬限制，非特权进程只能减小）
}
// 如果无法表示，则RLIM_SAVED_CUR和RLIM_SAVED_MAX分别是rlim_cur和rlim_max的返回值
```

fork创建的子进程会继承这些限制且在exec调用之间不会得到保持

### 资源限制细节

* RLIMIT_AS：限制进程的虚拟内存的最大字节数，影响brk、sbrk、mmap、mremap、shmat，超过返回ENOMEM错误
* RLIMIT_CORE：限制产生的core文件的大小，0表示阻止生成core文件
* RLIMIT_CPU：限制进程最多使用CPU的时间，超过返回SIGXCPU错误（每超过一秒发送一次，达到硬限制后发送SIGKILL信号）
* RLIMIT_DATA：限制进程数据段的大小，影响brk、sbrk，超过返回ENOMEM错误
* RLIMIT_FSIZE：限制进程能够创建的文件的大小，超过内核就会发送SIGXFSZ信号，系统调用write返回EFBIG错误
* RLIMIT_MEMLOCK：限制一个进程最多多少字节的虚拟内存能够锁进物理内存防止内存被交换出去，影响mlock、mlockall、mmap、shmctl
* RLIMIT_MSGQUEUE：限制能够为调用进程的真实用户ID的POSXI消息队列分配的最大字节
* RLIMIT_NICE：nice和sched_setscheduler能够为进程设置的最大nice值，通过公式20-rlim_cur计算而来
* RLIMIT_NOFILE：一个进程能够分配的最大文件描述符数量加1，由/proc/sys/fs/nr_open定义，失败的错误一般是EMFILE，但是在dup2中，返回错误是EBADF，fcntl返回错误是EINVAL；还有一个系统级限制，规定了所有进程能够打开的文件数量：/proc/sys/fs/file-max；Linux上还可以通过使用readdir扫描/proc/PID/fd目录下的内容检查一个进程当前打开的文件描述符
* RLIMIT_NPROC：限制了进程的真实用户ID下最多能够创建的进程数量，试图fork、vfork、clone超过限制返回EAGAIN错误；Linux特有的/proc/sys/kernel/thread-max表示所有用户能够创建的线程数量
* RLIMIT_RSS：Linux上没起作用
* RLIMIT_RTPRIO：限制了sched_setscheduler和sched_setparam能够为进程设置的最高实时优先级
* RLIMIT_RTTIME：规定了进程在实时调度策略中不睡眠（即执行阻塞式调用）情况下最大能消耗的CPU秒数，达到限制时行为与RLIMIT_CPU表现一样
* RLIMIT_SIGPENDING：限制了调用进程的真实用户ID下信号队列中最多容纳的信号（标准信号+实时信号）数量，超过返回EAGAIN错误
* RLIMIT_STACK：限制了进程栈的大小，试图扩展内核发送一个SIGSEGV信号