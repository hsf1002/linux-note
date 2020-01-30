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

