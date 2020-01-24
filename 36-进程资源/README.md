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

