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
// 若成功，返回信号量ID，若出错，返回-1
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

### 消息队列的限制

* MSGMNI：系统级，所能创建的消息队列标识符的数量（即消息队列的个数）
* MSGMAX：系统级，单条消息最多可写入的字节数（msgsnd，EINVAL）
* MSGMNB：系统级，一个消息队列中一次最多可以保存的字节数（msg_qbytes）
* MSGTQL：系统级，所有消息队列所能存放的消息总数
* MSGPOLL：系统级，所有消息队列的数据的缓冲池的大小

Linux特有的msgctl IPC_INFO操作能够获取一个类型为msginfo的结构，其中包含了各种消息队列的限制值

### 显示系统中所有消息队列

获取系统中IPC对象的方法，除了/proc下的一组文件外，就是Linux特有的ctl方法如msgctl：

* MSG_INFO、SEM_INFO、SHM_INFO
* MSG_STAT、SEM_STAT、SHM_STAT：与IPC_STAT操作一样，获取一个IPC对象的数据结构，差别是，这些操作的第一个参数是entries数组的下标，如果操作成功，返回下标对应的IPC对象的标识符

查找系统上所有消息队列的步骤：

1. 使用MSG_INFO查找消息队列的entries数组的最大下标
2. 执行循环，从0到最大下标之间的每个值都执行一个MSG_STAT操作

### System V 消息队列的缺陷

无分隔符的字节流：管道、FIFO、UNIX domain 流socket

由分隔符的消息：System V消息队列、POSIX消息队列、UNIX domain 数据报socket

System V消息队列与众不同的特性是能够为每个消息加上一个数字类型，读取进程可以根据类型读取，主要缺点：

* 通过标识符而不是文件描述符引用，即无法使用IO技术如select、poll
* 使用键而不是文件名标识消息队列增加了程序设计复杂性
* 消息队列无连接，内核不会像管道、FIFO以及socket那样维护引用队列的进程数，这将导致：
  * 程序何时能够安全的删除消息队列？
  * 程序如何保证不再使用的队列被删除？

* 消息队列的总数，消息的大小以及单个队列的容量都是有限的

总体来说，最好避免使用System V消息队列，POSIX消息队列是一种替代方案

