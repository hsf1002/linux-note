## 第45章 System V IPC介绍

### 概述

SUSv3因为遵循XSI要求实现System V IPC，因此也称为XSI IPC，通过CONFIG_SYSVIPC选项进行配置

![WechatIMG43.jpeg](https://i.loli.net/2020/01/30/IGkHQOD8AiUo9db.jpg)

##### 创建和打开一个IPC对象

msgget、semget、shmget，将完成下面的一个操作：

* 使用key创建一个新的对象，返回唯一的标识符
* 返回key对应的对象的标识符

一个进程可以通过指定IPC_EXCL标记确保它是创建IPC对象的进程

##### IPC对象的删除与持久

IPC_RMID适用于三种IPC类型，用来删除一个对象，对于信号量和消息队列而言，删除立即生效，对象包含的所有信息被销毁，不管是否有其他进程仍然在使用对象，共享内存对象的删除稍有不同，只有当使用该内存段的进程与该内存段分离之后才会删除（与文件的删除类似）。IPC对象具有持久性，一旦被创建，就一直存在直到被显式删除或系统关闭

### IPC key

key是一个整型值，数据类型是key_t

##### 使用IPC_PRIVATE产生唯一的key

在创建IPC对象时必须指定IPC_PRIVATE，无需指定IPC_CREAT和IPC_EXCL标记，这项技术对于父进程在执行fork之前创建IPC对象从而导致子进程继承该对象标识符的多进程程序中特别有用

##### 使用ftok产生唯一的key

ftok：file to key返回一个适合在后续对某个IPCget系统调用进行调用时使用的key值

```
#include <sys/ipc.h>

key_t ftok(char *pathname, int proj);
// 若成功，返回key，若出错，返回-1
```

SUSv3要求：

1. 算法只使用proj的低8位
2. 必须确保pathname引用一个可以stat的既有文件
3. ftok使用i-node号生成key，而非文件名

在可移植的程序上，应避免将proj置为0

Linux上ftok返回32位的key，由proj的低8位+文件所属的文件系统的次要设备号的低8位+pathname对应的i-node号的低16位组成，不同的文件可能产生的key相同，不过概率很小，因为不同文件系统上的两个文件的i-node号的最低16位可能相同，且两个不同的磁盘设备可能拥有同样的次要设备号

### 关联数据结构和对象权限

一旦IPC对象被创建，可以指定IPC_STAT操作获取该对象关联数据结构的一个副本，使用IPC_SET修改这个结构的部分数据，除了各种IPC特有数据外，三种包含同一个结构ipc_perm，即对象的权限信息：

```
struct ipc_perm
{
key_t  key;  /* 关键字 */
uid_t  uid;  /* 所有者的有效用户ID，可更改 */
gid_t  gid;  /* 所有者所属组的有效组ID*/
uid_t  cuid; /* 创建者的有效用户ID，不可更改 */
gid_t  cgid; /* 创建者所属组的有效组ID*/
unsigned short  mode; /* Permissions + SHM_DEST和SHM_LOCKED标志*/
unsigned short  seq;  /* 序列号*/
};
```

IPC对象的权限模型和文件比，有明显差异

* IPC对象只有读和写权限有意义，执行权限没有意义
* 权限检测根据进程的有效用户ID、有效组ID、以及辅助组ID进行，文件使用的是文件系统ID

### IPC标识符和客户端/服务器程序

通常是服务器创建IPC对象，而客户端使用它们，如果服务器进程奔溃或停止，重启后的服务器应该丢弃所有既有的客户端，删除之前服务器创建的IPC对象，并创建IPC对象的新实例，但是如果key相同，重新创建的IPC对象将返回同样的标识符，内核采用了一个算法，通常能够确保在创建新IPC对象时，即使key传入的一样，返回的标识符也不同

### IPC get调用使用的算法

对于每种IPC机制，内核都维护着一个关联的ipc_ids结构，记录着该IPC机制的所有实例的各种全局信息，在执行get时内核所采用的算法：

1. 在关联数据结构列表中搜索key字段与get调用中指定的参数匹配的结构
   * 如果没有找到匹配的结构，且没有指定IPC_CREAT，返回ENOENT错误
   * 如果找到了一个匹配的结构，且指定了IPC_CREAT和IPC_EXCL，返回EEXIST错误
   * 否则在找到一个匹配结构的情况下跳过下面的步骤

2. 如果没有找到匹配的结构，且指定了IPC_CREAT，则初始化一个新的数据结构，并更新ipc_ids中的各个字段
   * 传递给get的key被复制到新分配的结构的xxx_perm.__key字段
   * ipc_ids结构中的seq字段当前值被复制到关联数据结构的xxx_perm.__seq字段中，将seq的值加1

3. 使用下面的公式计算IPC对象的标识符

```
identifier = index + xxx_perm.__seq * SEQ_MULTIPLIER
// index是关联数据结构entries数组的下标，SEQ_MULTIPLIER是一个常数
```

### ipcs和ipcrm命令

ipcs可以获取系统上IPC对象的信息，ipcrm可以删除一个IPC对象

### 获取所有的IPC对象列表

Linux提供了两种获取系统所有IPC对象列表的非标准方法：

1. /proc/sysvipc目录
2. 使用Linux特有的ctl调用

/proc/sysvipc目录下由三个只读文件：

```
/proc/sysvipc/msg
/proc/sysvipc/sem
/proc/sysvipc/shm
```

### IPC限制

Linux上，ipcs -l可以列出各种IPC限制，程序使用LInux特有的IPC_INFO ctl操作获取同样的信息