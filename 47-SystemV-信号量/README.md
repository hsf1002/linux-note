## 第47章 System V 信号量

### 概述

信号量时内核维护的一个计数器（整数），用于同步进程的动作，为了获取共享资源，进程执行如下操作：

1. 测试控制该资源的信号量
2. 若信号量为正，则进程使用该资源，信号量值减1
3. 若信号量为0，则进程进入休眠状态，直至信号量值大于0，进程被唤醒，返回步骤1

进程不再使用由信号量控制的共享资源时，信号量值增1，如果有进程正在休眠等待此信号量，唤醒它们

这种形式的信号量为二元信号量，一般而言，初始值可以是任意正值，表示有多少个共享资源可供使用，信号量通常内核实现。XSI信号量要复杂得多，因为三个特性：

1. 信号量并非是单个非负值，而是含有一个或多个信号量值得集合
2. 信号量的创建独立于初始化，不能原子性的创建一个信号量集合并对其赋值
3. 即使没有使用各种形式的XSI IPC，它们仍然是存在的，有的程序终止时没有释放已经分配的信号量

### 数据结构

```
struct semid_ds 
{
    struct ipc_perm    sem_perm;        /* 对信号量操作的许可权 */
    __kernel_time_t    sem_otime;       /* 创建时初始化为0，调用semop成功或因SEM_UNDO改变时修改为当前时间 */
    __kernel_time_t    sem_ctime;       /* 创建时及每次IPC_SET、IPC_SETALL、IPC_SETVAL时修改为当前时间 */
    unsigned short     sem_nsems;       /* 数组中的信号量数 */

    struct sem    *sem_base;            /*指向第一个信号量 */
    struct sem_queue *sem_pending;      /* 等待处理的挂起操作 */
    struct sem_queue **sem_pending_last;/* 最后一个正在挂起的操作 */
    struct sem_undo    *undo;           /* 撤销的请求 */
};
```

### 创建或打开一个信号量集

```
#include <sys/sem.h>

int semget(key_t key, int nsems, int flag);
// 若成功，返回信号量集标识符，若出错，返回-1
// 若创建一个新集合，必须指定nsems，且大于0
// 若引用现有集合，将nsems指定为0
// 无法修改一个既有集中信号量个数

flag的取值：
IPC_CREAT：若key相关的信号量集不存在，则创建
IPC_EXCL：若key相关的信号量存在且指定了IPC_CREAT，返回EEXIST错误
```

### 控制操作

```
int semctl(int semid, int semnum, int cmd, .../* union semun arg */);
// 返回值，见下
// 若是操作单个信号量，semnum是信号量集的数组索引，其他操作忽略此参数

arg是联合，而非联合的指针
union semun
{
    int val;
    struct semid_ds *buf;
    unsigned short *array;
}

cmd的含义如下：
常规操作---忽略semnum参数
IPC_STAT：对此集合取semid_ds结构，存储在arg.buf指向的结构
IPC_SET：对信号量的属性进行设置
IPC_RMID：删除semid指定的信号量集合semid_ds结构，所有因semop调用中等待这个集合的信号量而阻塞的进程立刻唤醒
获取和初始化信号量值---
GETVAL：返回信号量集semnum指定信号量的值
GETALL：返回信号集量中所用信号量的值
SETVAL：设置信号量集中semnum指定的信号量的值
SETALL：设置信号量集中所用信号量的值
获取单个信号量的信息---
GETPID：返回最后一个执行semop操作的进程ID
GETNCNT：返回正在等待资源的进程的数量
GETZCNT：返回正在等待完全空闲资源的进程的数量

除了GETALL以外的所有GET命令，返回相应值，其他命令，若成功，返回0，若出错，返回-1
```

### 信号量操作

```
#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/sem.h>

int semop(int semid, struct sembuf semoparray[], size_t nops);
// 若成功，返回0，若出错，返回-1
// 具有原子性，或者执行数组中所有操作，或者一个也不做

semoparray指向一个有sembuf结构表示的信号量操作数组
struct sembuf
{
    unsigned short sem_num;
    short sem_op;	  /* operatioin(negative, 0, or positive */
    short sem_flg;	/* IPC_NOWAIT, SEM_UNDO */
}

sem_op为正：表示进程释放的占用的资源数，sem_op的值会加到信号量值上，若指定了SEM_UNDO，则从信号量调整值减去sem_op
sem_op为负：表示要获取由该信号量控制的资源，若信号量值大于等于sem_op绝对值，则从信号量值中减去sem_op绝对值，若指定了SEM_UNDO，则sem_op的绝对值加到信号量调整值上
sem_op为0：表示调用进程希望等待到该信号量值变成0，如果是0立即结束，否则一直阻塞


int semtimedop(int semid, struct sembuf semoparray[], size_t nops, struct timespec *timeout);
// 若成功，返回0，若出错，返回-1
// 通过timeout设置阻塞的时间上限，如果设置为NULL，与semop一样
```

### 多个阻塞信号量操作的处理

* 如果多个因减小一个信号量值而发生阻塞的进程对该信号量减去的数值一样，则当条件满足时哪个进程最先唤醒时不确定的
* 如果多个因减小一个信号量值而发生阻塞的进程对该信号量减去的数值不一样，则最先满足条件的进程先唤醒

### 信号量撤销值

如果一个进程在调整完信号量值后终止了（正常或异常），默认情况下信号量的值不会发生变化，其他因等待该信号量变更后，而被阻塞的进程可能无法再唤醒，为了避免此问题，通过semop时指定SEM_UNDO标记，内核将在进程终止时撤销该信号量的变更；内核记录一个进程在一个信号量上使用SEM_UNDO所做出的调整总和semadj，在进程终止时，减去这个总和即可

通过fork创建的子进程不会继承semadj，该值会在exec期间得到保持

##### SEM_UNDO的限制

它其实并没有看上去那么有用，因为信号量通常对应于请求或释放一些共享资源，进程终止时很难将一个进程的共享资源原子性的返回到一致的状态，还有就是有些情况下，进程终止时无法对信号量进行调整

### 二元信号量协议

以System V信号量为基础实现的一个更简单的协议，二元信号量有两个值和两种操作：

* 预留（使用中）：如果信号量已经被一个进程预留了，则阻塞
* 释放（可用）：释放一个当前被预留的信号量，这样其他进程就可以预留了

这两种操作通常称为P和V，或down和up，POSIX将其定义为wait和post

有时候需要第三个操作：

* 有条件的预留：如果信号量已经被一个进程预留了，立即返回状态不可用

### 信号量的限制

* SEMAEM：semadj总和的最大值（semop，ERANGE）
* SEMMNI：系统级，所能创建的信号量标识符的数量，信号量集的个数（semget，ENOSPC）
* SEMMSL：一个信号量集所能分配的最大数量（semget，EINVAL）
* SEMMNS：系统级，所有信号量集上信号量数量（semget、ENOSPC）
* SEMOPM：每个semop调用能够执行操作的最大数量（E2BIG）
* SEMVMX：一个信号量值的最大值（semop，ERANGE）
* SEMMNU：系统级，信号量撤销结构的总数量
* SEMUME：每个信号量撤销结构中撤销条目的最大值

Linux特有的/proc/sys/kernel/sem中有些值可以修改

Linux特有的semctl IPC_INFO操作能够获取一个类型为seminfo的结构，其中包含了各种的限制值

### System V 信号量的缺陷

主要缺点：

* 通过标识符而不是文件描述符引用，即无法使用IO技术如select、poll
* 使用键而不是文件名标识信号量增加了程序设计复杂性
* 创建和初始化是单独的系统调用，意味着需要编程防止竞争条件
* 内核不会维护一个信号量集的进程数量，何时删除一个信号量集合如何删除不再使用的信号量集？
* System V提供的接口过于复杂
* 信号量的操作存在诸多限制

总体来说，替代System V信号量的方案不多，一种方式是记录锁

