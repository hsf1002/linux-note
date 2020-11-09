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
SCHED_RR: 实时循环
SCHED_FIFO: 实时先入先出
SCHED_IDLE: 与SCHED_OTHER类似
SCHED_BATCH: 与SCHED_OTHER类似，用于批量执行
SCHED_OTHER: 标准的循环时间共享
```

##### 修改和获取策略和优先级

修改进程pid的调度策略和优先级：

```
#include <sched.h>

int sched_setscheduler(pid_t pid, int policy, const struct sched_param *param);
// 若成功，返回0，若出错，返回-1
// 对于SCHED_RR和SCHED_FIFO，param必须是sched_get_priority_min和sched_get_priority_max之间，对于其他，必须是0
// 若成功，会将pid指定的进程移到与其优先级对应的队列队尾
// fork创建的子进程会继承父进程的调度策略和优先级，且在exec中保持

struct sched_param
{
    int sched_priority;
}
```

sched_setparam提供了一个功能子集，仅修改优先级：

```
int sched_setparam(pid_t pid, const struct sched_param *param);
// 若成功，返回0，若出错，返回-1
```

获取进程的调度策略和优先级：

```
int sched_getscheduler(pid_t pid);
// 若成功，返回调度策略，若出错，返回-1
int sched_getparam(pid_t pid, struct sched_param *param);
// 若成功，返回0，param返回优先级，若出错，返回-1

// 如果pid是0，则返回调用进程的信息
```

##### 防止实时进程锁住系统

由于SCHED_RR和SCHED_FIFO进程会抢占低优先级的进程，因避免一直占用CPU从而锁住系统的情况：

* 使用setrlimit设置一个合理的低软CPU时间组员限制，如果进程消耗太多CPU，会收到SIGXCPU，默认终止进程
* 使用alarm设置警报定时器，如果超出，SIGALRM会终止进程
* 创建一个拥有高实时优先级的看门狗进程，无限循环监控其他进程的状态（主要是CPU消耗量）
* 非标准的资源限制RLIMIT_RTTIME，限制了一个进程在不执行阻塞式系统调用能够消耗的CPU时间，单位毫秒

##### 子进程的调度策略

如果指定了SCHED_RESET_ON_FORK标记，fork时创建的子进程不会继承特权调度策略和优先级，规则：

* 如果调用进程的调度策略是SCHED_RR和SCHED_FIFO，子进程的策略被重置为SCHED_OTHER
* 如果进程的nice值为负值（高优先级），子进程的nice被重置为0

一旦启用该标记，只有特权进程（CAP_SYS_NICE）才能禁用此标记

##### 释放CPU

实时进程通过两种方式自愿释放CPU：阻塞式系统调用如read或：

```
int sched_yield(void);
// 若成功，返回0，若出错，返回-1
// 如果存在与调用进程相同的排队的其他可运行的进程，调用进程放在队尾，队头进程开始运行，如果队列中没有可运行的进程，那么sched_yield不会做任何事情，调用进程继续使用CPU
```

##### SCHED_RR时间片

返回SCHED_RR每次被授权使用CPU时分配的时间片长度：

```
int sched_rr_get_interval(pid_t pid, struct timespec *tp);
// 若成功，返回0，若出错，返回-1
```

### CPU亲和力

为了防止高速缓冲器不一致，多处理器架构在某个时刻只允许数据被存放在一个CPU的高速缓冲器中，软CPU亲和力在条件允许的情况下进程尽量调度到原来的CPU上运行，可以为进程设置硬CPU亲和力，限制其在某个或某几个CPU上运行，原因是：

* 避免由高速缓冲器的数据失效带来的性能影响
* 多线程中可能提升性能
* 对于时间关键的应用程序而言，可能需要为此应用预留一个或更多CPU，而将系统中大多数进程限制在其他CPU上

设置pid指定的进程的CPU亲和力：

```
#define _GNU_SOURCE
#include <sched.h>

int sched_setaffinity(pid_t pid, size_t len, cpu_set_t *set);
// 若成功，返回0，若出错，返回-1
// 若pid是0，则指调用进程本身
// CPU亲和力实际是线程属性，pid可以指定为gettid()
```

cpu_set_t是一个位掩码，应将其看成不透明的结构，所有这个结构的操作都应该使用宏：

```
void CPU_ZERO(cpu_set_t *set);        // 将set初始化为空
void CPU_SET(int cpu, cpu_set_t *set);// 将cpu添加到set
void CPU_CLR(int cpu, cpu_set_t *set);// 将cpu从set删除

int CPU_ISSET(int cpu, cpu_set_t *set);// cpu是否属于set
// 若成功，返回1，若出错，返回0
```

获取pid指定的进程亲和力掩码：

```
int sched_getaffinity(pid_t pid, size_t len, cpu_set_t *set);
// 若成功，返回0，若出错，返回-1
// 如果CPU亲和力没被修改，则返回系统所有CPU集合
// 执行时不会进行权限检查，非特权及成年后能够获取所有进程的CPU亲和力掩码
```

fork创建的子进程会继承父进程的CPU亲和力掩码且在exec调用之间保留