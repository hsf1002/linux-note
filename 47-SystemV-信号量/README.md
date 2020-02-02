## 第47章 System V 信号量

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
int msgctl(int msqid, int cmd, struct msgid_ds *buf);
// 若成功，返回0，若出错，返回-1

cmd参数指定对msgid的队列要执行的命令，这三条命令也适用于信号量和共享存储
IPC_STAT：取队列的msqid_ds结构，放在buf中
IPC_SET：将字段msg_perm.uid、msg_perm.gid、msg_perm.mode和msg_qbytes从buf指向的结构复制到msqid
IPC_RMID：删除消息队列及其数据，立刻生效，队列中剩余消息都会丢失，所有被阻塞的读者和写者进程会立刻醒来，忽略第三个参数
```

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

