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

##### 文件描述符何时就绪

如果对IO函数的调用不会阻塞（未指定O_NONBLOCK标记），而不论是否能够实际传输数据，此时文件描述符就被认为是就绪的，select和poll只会告诉我们IO操作是否会阻塞

普通文件：

```
select: 总是可读和可写
poll：会在revents字段返回POLLIN和POLLOUT标志
```

终端和伪终端：

```
条件或事件                             select    poll
有输入                                  r       POLLIN
可输出                                  w       POLLOUT  
伪终端对端调用close后                     rw    取决于实现，Linux至少有POLLHUP    
信号模式下的伪终端主设备检测到从设备状态改变   x       POLLPRI  
```

管道和FIFO：

```
读端(假定revents字段已经指定了POLLIN)：
         条件或事件
管道中有数据？  写端打开了？    select    poll
  N             N             r     POLLHUP
  Y             Y             r      POLLIN
  Y             N             r   POLLIN|POLLHUP
  
写端(假定revents字段已经指定了POLLOUT)：
             条件或事件
有PIPE_BUF字节空间？  读端打开了？    select    poll
  N                    N             w     POLLERR
  Y                    Y             w      POLLOUT
  Y                    N             w   POLLOUT|POLLERR
```

套接字：

```
(假定revents字段已经指定了POLLIN|POLLOUT|POLLPRI)
条件或事件            select    poll
有输入                 r       POLLIN
可输出                 w       POLLOUT
在监听套接字上建立连接    r       POLLIN
接收到带外数据（限TCP）   x       POLLPRI
流式套接字的对端关闭连接  rw  POLLIN|POLLOUT|POLLRDHUP
或执行了shutdown
```

##### 比较select和poll

* 实现细节：Linux内核层面，select和poll都使用了相同的内核poll例程集合

* API之间的区别：
  * select使用的fd_set有一个上限FD_SETSIZE，poll本质上没有上限
  * select的参数fd_set同时也是保存结果的地方，如果要循环调用select，需要每次重新初始化fd_set，poll通过两个独立的字段events和revents单独处理
  * select提供的超时精度（微秒）比poll提供的毫秒要高
  * 如果一个文件描述符关闭了，在对应的revents字段设定POLLNVAL标记，poll会准确的告诉我们时哪个文件描述符，而select只会返回-1，并设错误码EBADF

* 性能：如果满足如下条件之一，则select和poll具有相似的性能表现
  * 待检查的文件描述符数量较小
  * 有大量的文件描述符待检查，但是分布得很密集

##### select和poll存在的问题

* 每次调用select或poll，内核必须检查所有指定的文件描述符，看它们是否处于就绪状态
* 每次调用select或poll，程序必须传递一个表示所有被检查的文件描述符的数据结构到内核，检查后，修改这个数据结构再返回给程序
* select或poll调用完毕，程序必须检查返回的数据结构的每个元素，来判断哪个文件描述符处于就绪状态

随着待检查的文件描述符数量增加，select和poll所占用的CPU也随之增加，信号驱动IO和epoll可以使内核记录下进程中感兴趣的文件描述符，通过这种机制消除了了select和poll的性能延展问题

