## 第63章 其他备选的IO模型

### 概述

三种可选的IO模型

* IO多路复用：允许进程同时检查多个文件描述符以找出任何一个是否可执行IO操作，select/poll，可移植性好，缺点是同时检查大量（数百上千）文件描述符时性能延展性不佳
* 信号驱动IO：当有输入或者数据可以写到指定文件描述符上，内核向请求数据的进程发送一个信号
* epoll API：Linux专有的特性，既弥补了select/poll的缺点，又避免了处理信号的复杂，可以指定想要检查的事件类型（读就绪或写就绪），可以选择水平触发或边缘触发的形式通知进程

它们都是用来实现同一个目标的技术，即同时检查多个文件描述符，看它们是否准备好进行IO操作，libevent库是一个软件层，可以以透明的方式描述任意一种技术：select、poll、信号驱动IO、epoll

##### 水平触发和边缘触发

* 水平触发通知：如果文件描述符上可以非阻塞的执行IO系统调用，允许我们在任意时刻重复检查IO状态，没必要每次当文件描述符准备就绪时尽可能多的执行IO
* 边缘触发通知：如果文件描述符自上次检查以来有了新的IO活动，比如新的输入，应该按照如下方式设计
  * 接收到一个IO事件通知后，应该尽可能多的执行IO，如尽可能多的读取字节，否则可能失去执行IO的机会
  * 每个需要检查的文件描述符都应该设置为非阻塞模式，得到IO事件通知后重复执行IO操作，直到系统调用，如read、write以错误码EAGAIN或EWOULDBLOCK的形式失败

```
IO模式      水平触发      边缘触发
select/poll  Y             N
信号驱动IO    N             Y
epoll        Y             Y
```

### IO多路复用

select首次出现在BSD系统的套接字API中，poll应该更为广泛，出现在System V中，可以在普通文件、终端、伪终端、管道、FIFO、套接字以及其他字符型设备上使用select或poll检查文件描述符

##### 系统调用select

```
#include <sys/select.h>

int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *errorfds, struct timeval *timeout);
// 返回值：若成功，返回准备就绪的文件描述符的个数（每个返回的文件描述符集合都需要FD_ISSET检查，如果一个文件描述符在三个集合同时被指定，且多个IO事件都处于就绪的话，则会统计多次），超时返回0，若出错返回-1（EBADF表示文件描述符非法，EINTR表示调用被信号中断）
// readfds, writefds, errorfds分别表示输入是否就绪、输出是否就绪、异常（1. 连接到处于信包模式下的伪终端主设备上的从设备状态发生了变化 2. 流式套接字上接收到了带外数据）是否发生的文件描述符集合
// nfds比三个文件描述符集合所包含的最大值还要大1
// 在缺少亚秒级sleep如nanosleep的系统中，select可以通过将nfds设为0，readfds, writefds, errorfds设为NULL，期望的休眠时间再timeout中指定来实现
```

```
// 将fdset初始化为空
void FD_ZERO(fd_set *fdset)
// 将fd添加到fdset中
void FD_SET(fd, fd_set *fdset);
// 将fd从fdset中移除
void FD_CLR(fd, fd_set *fdset);
// fd是否是fdset中成员
int FD_ISSET(fd, fd_set *fdset);
// 将fdset_orig拷贝到fdset_copy
void FD_COPY(fd_set *fdset_orig, fd_set *fdset_copy);

文件描述符最大值由常量FD_SETSIZE决定
```

##### 系统调用poll

```
#include <poll.h>

int poll(struct pollfd fds[], nfds_t nfds, int timeout);
// 返回值：若成功返回准备就绪的文件描述符个数，若超时返回0，若出错返回-1
// 与select不同的是，即使同一个文件描述符在revents中设定了多个位掩码，返回的文件描述符集合中也不会统计多次

struct pollfd 
{
  int    fd;       /* 文件描述符 */
  short  events;   /* 监控的事件 */
  short  revents;  /* 发生的事件 */
};

位掩码      events的输入    revents的返回    描述
POLLIN        Y               Y        可读取非高优先级数据
POLLRDNORM    Y               Y        同POLLIN
POLLRDBAND    Y               Y        可读取高优先级数据（Linux中不可用）
POLLPRI       Y               Y        可读取高优先级数据
POLLRDHUP     Y               Y        对端套接字关闭

POLLOUT       Y               Y        普通数据可写
POLLWRNORM    Y               Y        POLLOUT
POLLWRBAND    Y               Y        优先级数据可写

POLLERR                       Y        有错误发生
POLLHUP                       Y        出现挂断
POLLNVAL                      Y        文件描述符未打开

POLLMSG                                Linux中不使用

需要定义_XOPEN_SOURCE测试宏：POLLRDNORM、POLLRDBAND、POLLWRNORM、POLLWRBAND
需要定义_GNU_SOURCE测试宏：  POLLRDHUP
```

