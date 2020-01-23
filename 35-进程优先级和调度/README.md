## 第35章 进程优先级和调度

### 进程优先级

默认情况下，每个进程轮流使用CPU直到CPU时间片被用光或者自己主动放弃CPU（睡眠或读取磁盘操作）

nice取值：-20（高）----19（低），默认是0

nice值是一个权重因素，导致内核调度器倾向于调度拥有高优先级的进程，低优先级进程的CPU时间会变少

只有特权进程才能赋一个负优先级，非特权进程只能降低自己的优先级，这么做才算是对其他进程“友好了”

fork创建子进程会继承nice值并在exec调用中得到保持

```
#include <sys/resource.h>

int getpriority(int which, id_t who);
// 若成功，返回20-nice，若出错，返回-1
// 若有多个进程符合要求（PRIO_PGRP或PRIO_USER时），返回优先级最高的
// 特权进程可以修改任意进程，非特权进程只能修改自己（PRIO_PROCESS）或目标进程的优先级（前提是自己的有效用户ID与目标进程的真实或有效用户ID匹配）

int setpriority(int which, id_t who, int prio);
// 若成功，返回0，若出错，返回-1

which的取值：
PRIO_PROCESS：操作进程ID为who的进程，如果who为0，表示调用进程本身
PRIO_PGRP：操作进程组ID为who的进程组的所有成员，如果who为0，表示调用进程所属的进程组
PRIO_USER：操作所有真实用户ID为who的进程，如果who为0，那么使用调用进程的真实用户ID
```

自内核2.6.12开始，非特权进程可以提升nice值，可以提高到20-rlim_cur指定的值，rlim_cur是RLIMIT_NICE的软资源限制，假如其是25，则可提升nice值到-5

