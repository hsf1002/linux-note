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

