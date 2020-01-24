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

### 实时进度调度策略

##### SCHED_RR

优先级相同的进程以循环时间分享的方式进行，进程会保持对CPU的控制直到下面一个条件得到满足：

1. CPU时间片到了
2. 自愿放弃CPU，如阻塞式调用、sleep或yield
3. 进程终止
4. 被一个高优先级进程抢占了

丢掉CPU的控制后会被放置到与其优先级级别对应的队列的队尾

SCHED_RR：存在严格的优先级级别，高优先级的进程总是先运行

SCHED_OTHER：高优先级的进程不会独占CPU，仅在调度时提供一个较大的权重

##### SCHED_FIFO

不存在时间片，一旦获得满足，就会一直运行直到下面一个条件满足：

1. 自动放弃CPU
2. 进程终止
3. 被一个高优先级进程抢占了

第一种情况会被放置到与其优先级级别对应的队列的队尾，最后一种情况会继续运行

SCHED_RR和SCHED_FIFO中被抢占的原因：

* 之前被阻塞的高优先级进程解除阻塞了
* 另一个进程的优先级被提高到高于当前进程
* 当前进程的优先级被降低到低于其他可运行的进程

##### SCHED_BATCH和SCHED_IDLE

SCHED_BATCH：与SCHED_OTHER类似，差别在于它会导致频繁被唤醒的任务被调度的次数较少

SCHED_IDLE：与SCHED_OTHER类似，差别在于nice值毫无意义，它运行低优先级的任务，这些任务在系统中没有其他任务使用CPU时才会被调度

### 实时进程调用API

##### 实时优先级范围

```
#include <sched.h>

int sched_get_priority_min(int policy); // 返回指定策略的最小优先级
int sched_get_priority_max(int policy); // 返回指定策略的最大优先级
// 若成功，返回非负整数，若出错，返回-1
// policy指的是调度策略
```

