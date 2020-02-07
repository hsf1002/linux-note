## 第52章 POSIX消息队列

### 概述

POSIX消息队列与System V消息队列的差别：

* 对象引用计数，可以更为安全的删除对象
* 与System V消息类型不同，POSIX消息有一个关联的优先级，消息之间是严格的按照优先级排队接收
* 提供了一个特性允许队列中的一条消息可异步通知进程

可选组件，通过CONFIG_POSIX_MQUEUE配置

### 打开、关闭、断开链接

##### 创建打开一个消息队列

```
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>

mqd_t mq_open(const char *name, int oflag, ... /*mode_t mode, struct mq_attr *attr */);
// 若成功，返回消息队列标识符，若出错，返回-1
// name一般是/mq_object
// oflag的取值：
O_CREAT: 队列不存在则创建，如果指定了，就需要后面两个参数mode和attr
O_EXCL: 与O_CREAT一起排他的创建队列，若同时指定两个标记且队列已经存在，调用失败
O_RDONLY: 只读打开
O_WRONLY: 只写打开
O_RDWR: 读写打开
O_NONBLOCK: 非阻塞打开
```

##### 关闭一个消息队列

```
int mq_close(mqd_t mqdes);
// 若成功，返回0，若出错，返回-1
// 如果调用进程在队列注册了消息通知，则通知自动被删除
// 与close一样，关闭消息队列并不会删除它
```

##### 删除一个消息队列

```
int mq_unlink(const char *name);
// 若成功，返回0，若出错，返回-1
// unlink在消息队列上的版本
```

fork的子进程会接收父进程的消息队列描述符的副本，不会继承任何消息通知，执行exec时，所有打开的消息描述符都会被关闭

### 描述符和消息队列之间的关系

POSIX消息队列在Linux被实现成了虚拟文件系统中的i-node，消息队列描述符和打开着的消息队列之间的关系与文件描述符与打开着的文件之间的关系类似

### 消息队列特性

```
struct mq_attr
{
  long int mq_flags;    // 消息队列的标志：0或O_NONBLOCK,用来表示是否阻塞 
  long int mq_maxmsg;   // 消息队列的最大消息数
  long int mq_msgsize;  // 消息队列中每个消息的最大字节数
  long int mq_curmsgs;  // 消息队列中当前的消息数目
  long int __pad[4];
};
```

##### 获取消息队列特性

```
mqd_t mq_getattr(mqd_t mqdes, struct mq_attr *attr);
// 若成功，返回0，若出错，返回-1
```

##### 修改消息队列特性

```
mqd_t mq_setattr(mqd_t mqdes, struct mq_attr *newattr, struct mq_attr *oldattr);
// 若成功，返回0，若出错，返回-1
// 如果oldattr不为NULL，就返回之前的特性
```

### 交换消息

##### 发送消息

```
int mq_send(mqd_t mqdes, const char *msg_ptr, size_t msg_len, unsigned msg_prio);
// 若成功，返回0，若出错，返回-1
// msg_len指定了msg_ptr指向的消息的长度，必须小于等于队列的mq_msgsize，长度为0是允许的
// msg_prio是非负的优先级，0表示优先级最低，无需使用优先级，指定为0即可
```

##### 接收消息

```
ssize_t mq_receive(mqd_t mqdes, char *msg_ptr, size_t msg_len, unsigned *msg_prio);
// 若成功，返回成功接收的字节数，若出错，返回-1
// 删除一条优先级最高，存在时间最长的消息
// 如果消息队列为空，一直阻塞直到有可用的消息，启用标记O_NONBLOCK则立即失败返回EAGAIN错误
```

##### 设置超时

```
#define _XOPEN_SOURCE 600
#include <mqueue.h>
#include <time.h>

int mq_timedsend(mqd_t mqdes, const char *msg_ptr,
                      size_t msg_len, unsigned msg_prio,
                      const struct timespec *abs_timeout);
// 若成功，返回0，若出错，返回-1
// 如果要指定一个相对的时长，可以使用clock_gettime来获取CLOCK_REALTIME时钟的当前值并加上时长
 
mqd_t mq_timedreceive(mqd_t mqdes, char *msg_ptr,
                      size_t msg_len, unsigned *msg_prio,
                      const struct timespec *abs_timeout);
// 若成功，返回成功接收的字节数，若出错，返回-1
// Linux上如果abs_timeout为NULL，表示永远不会超时
```

### 消息通知

注册调用进程在一条消息进入描述符mqdes引用的空队列时接收通知：

```
int mq_notify(mqd_t mqdes, const struct sigevent *notification);
// 若成功，返回0，若出错，返回-1
```

* 任何时刻都只有一个进程（注册进程）能够向一个特定的消息队列注册接收通知，如果消息队列已经存在注册进程了，则失败返回EBUSY错误
* 只有当一条新消息进入之前为空的队列时注册进程才会收到通知，如果在注册的时候队列已经有消息，当队列消息被清空后又有新消息到达时才发通知
* 注册进程发送一个通知之后就会删除注册消息，之后任何进程就能够向队列注册接收通知了，如果一个进程要持续的接收通知，必须每次接收到通知后再次注册
* 注册进程只有在当前不存在其他在该队列上调用mq_receive而发生阻塞的进程时才会收到通知，如果其他进程在mq_receive阻塞了，那么该进程会读取消息，注册进程会保持注册状态
* 一个进程可以通过传入notification为NULL撤销自己在消息通知上的注册信息

```
union sigval 
{
	/* Members as suggested by Annex C of POSIX 1003.1b. */
	int	sival_int;
	void	*sival_ptr;
};

struct sigevent
{
  int sigev_notify;         // 通知方式：SIGEV_NONE、SIGEV_SIGNAL、SIGEV_THREAD
  int sigev_signo;          // 如果是SIGEV_SIGNAL，表示信号编号
  union sigval sigev_value; // 如果是SIGEV_THREAD，表示传入参数
  
  void (*sigev_notify_func)(union sigval); // 函数指针
  void *sigev_notify_attributes;           // really 'pthread_attr_t'
}
```

### Linux特性

##### 通过命令行显示和删除消息队列对象

为了使用ls和rm列出和删除POSIX消息队列就必须使用如下命令将其挂载到文件系统：

```
mount -t mqueue source target
如：
su
mkdir /dev/mqueue
mount -t mqueue none /dev/mqueue
```

显示新挂载的记录和权限：

```
cat /proc/mounts | grep mqueue
ls -ld /dev/mqueue
```

创建消息队列后即可以ls和rm：

```
./pmsg_create -c /newq
ls /dev/mqueue
rm /dev/mqueue/newq
```

##### 获取消息队列相关信息

```
./pmsg_create -c /mq
./pmsg_send /mq abcdefg
cat /dev/mqueue/mq

./mq_notify_sig /mq &
cat /dev/mqueue/mq

kill %1

./mq_notify_thread /mq &
cat /dev/mqueue/mq
```

##### 使用另一种IO模型操作消息队列

Linux上消息队列描述符实际上是一个文件描述符，因此可以使用IO多路复用系统调用来监控这个文件描述符

### 消息队列限制

* MQ_PRIO_MAX：一条消息的最大优先级
* MQ_OPEN_MAX：一个进程最多能打开的消息队列数量

Linux特有的/proc/sys/fs/mqueue下三个值：

* msg_max：mq_maxmsg的上限
* msgsize_max：mq_msgsize的上限
* queues_max：系统级，系统最多能创建的消息队列的数量，一旦达到，就只有特权进程（CAP_SYS_RESOURCE）才能创建新队列

Linux特有的RLIMIT_MSGQUEUE为属于调用进程的真实用户ID的所有消息队列消耗的空间提供了上限

### POSIX和System V消息队列比较

优势：

* 消息通知特性允许一个进程能够在一条消息进入前为空的队列时异步的通过信号或线程来接收通知
* 可以使用poll、select等监控消息队列

劣势：

* 可移植性差
* System V消息队列根据消息类型选择消息的功能灵活性更强