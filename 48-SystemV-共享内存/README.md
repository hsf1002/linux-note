## 第48章 System V 共享内存

### 概述

共享内存不由内核控制，意味着需要某种同步机制使得进程不会同时访问共享内存，它允许两个或多个进程共享一个给定的存储区，因为数据不需要在客户进程和服务器进程之间复制，这是最快的一种IPC

### 数据结构

```
struct shmid_ds 
{
    struct ipc_perm        shm_perm;     /* 操作许可 */
    int                    shm_segsz;    /* 共享内存大小，字节为单位 */
    __kernel_time_t        shm_atime;    /* 创建时初始化为0，调用shmat后设置为当前时间 */
    __kernel_time_t        shm_dtime;    /* 创建时初始化为0，调用shmdt后设置为当前时间 */
    __kernel_time_t        shm_ctime;    /* 创建或每次IPC_SET后设置为当前时间 */
    __kernel_ipc_pid_t    shm_cpid;      /* 创建共享内存的PID */
    __kernel_ipc_pid_t    shm_lpid;      /* 创建时初始化为0，调用shaat或shadt后设置为调用进程的PID */
    unsigned short        shm_nattch;    /* 当前使用该共享内存的进程数量，调用shmat递加，调用shmdt递减 */
    unsigned short        shm_unused;    /* compatibility */
    void             *shm_unused2;    /* ditto - used by DIPC */
    void             *shm_unused3;    /* unused */
};
```

### 创建或打开一个共享内存

```
#include <sys/shm.h>

int shmget(key_t key, size_t size, int flag);
// 若成功，返回共享存储ID，若出错，返回-1
// size是共享存储段的长度，如果创建，必须指定size，且其内容初始化为0，如果引用，则将size指定为0

flag的取值：
IPC_CREAT：若key相关的信号量集不存在，则创建
IPC_EXCL：若key相关的信号量存在且指定了IPC_CREAT，返回EEXIST错误
SHM_HUGETLB：特权（CAP_IPC_LOCK）进程使用此标记创建一个使用huge page的共享内存，可降低硬件内存管理单元的超前转换缓冲器
SHM_NORESERVE：与MAP_NORESERVE在mmap中所起的作用一样
```

### 控制操作

```
int shmctl(int shmid, int cmd, struct shmid_ds *buf);
// 若成功，返回0，若出错，返回-1

cmd的含义如下：
IPC_STAT：读取共享内存区的shmid_ds机构，并将其存储到buf指向的地址
IPC_RMID：从系统中删除由shmid指向的共享内存区，以及数据结构shmid_ds，如果当前无进程附加该段，执行删除操作，否则等所有进程已经与该段分离（即shm_nattch字段为0）之后再删除
IPC_SET：设置共享内存的shmid_ds结构

Linux和Solaris还提供了另外两个命令
SHM_LOCK：对共享存储加锁，锁进内存RAM
SHM_UNLOCK：对共享存储解锁，允许它被交换出去
```

### 使用共享内存

```
void *shmat(int shmid, const void *addr, int flag);
// 若成功，返回指向共享存储段的指针，若出错，返回-1

addr==0：内核选择的第一个可用地址上，推荐的方式
addr非0且没有指定SHM_RND：连接到addr指定的地址(SHM_RND的意思是取整)
addr非0且指定了SHM_RND：连接到addr mod SHMLBA(shared memory low boundary)所表示的地址

flag指定为
SHM_RDONLY：表示只读，试图更新导致SIGSEGV信号，否则以读写方式连接
SHM_REMAP：指定之后addr必须是非0
```

### 分离共享内存

```
int shmdt(const void *addr);
// 若成功，返回0，若出错，返回-1
// addr是shmat的返回值

分离与shmctl的IPC_RMID删除不同
```

### 共享内存的限制

* SHMMNI：系统级，所能创建的共享内存标识符数量（shmget，ENOSPC）
* SHMMIN：共享内存段的最小大小，被定义为1（shaget，EINVAL）
* SHMMAX：共享内存段的最大大小，依赖于可用RAM和交换空间（shmget，EINVAL）
* SHMALL：系统级，限制了共享内存内的分页总数，依赖于可用RAM和交换空间（shmget，ENOSPC）
* SHMSEG：系统级，限制了所能附加的共享内存段数量

Linux特有的/proc/sys/kernel/shmmni等可查看

Linux特有的shmctl IPC_INFO操作能够获取一个类型为shminfo的结构，其中包含了各种限制值

