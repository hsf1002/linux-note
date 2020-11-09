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

### 信号驱动IO

当文件描述符上有可执行IO操作时，进程请求内核为自身发送一个信号，信号驱动IO可以使用在普通文件、终端、伪终端、管道、FIFO、套接字、字符型设备，以及inotify的文件描述符上；由于接收到SIGIO信号的默认行为是终止进程，应该先为其安装处理例程

##### 何时发送IO就绪的信号

* 终端和伪终端：产生新的输入时会生成一个信号，即使之前的输入没有读取，如果终端出现“文件结尾“，会发送”输入就绪“的信号，但是伪终端不会；对终端来说没有”输出就绪“的信号，但是当伪终端主设备侧读取了输入后会产生“输出就绪”的信号

* 管道和FIFO：对于管道和FIFO的读端，信号将在如下情况产生：

  * 数据写入到管道
  * 管道的写端关闭

  对于管道和FIFO的写端，信号将在如下情况产生：

  * 对管道的读取增加了管道的空余空间大小，因此可以写入PIPE_BUF个字节而不被阻塞
  * 管道的读端关闭

* 套接字：UNIX和Internet域下的数据报套接字，信号在如下情况产生：

  * 一个输入数据报到达
  * 套接字上发生了异步错误

  UNIX和Internet域下的流式套接字，信号在如下情况产生：

  * 监听套接字上接收到了新的连接
  * TCP connect请求完成，进入到ESTABLISHED状态，对于UNIX域套接字，不会发出信号
  * 套接字上接收到了新的输入
  * 套接字对端使用shutdown关闭了写连接（半关闭），或者通过close（完全关闭）
  * 套接字上输入就绪
  * 套接字上发生了异步错误

* inotify文件描述符：可读状态时

##### 优化信号驱动IO的使用

信号驱动IO的高性能是因为内核可以记住要检查的文件描述符，且仅当IO事件实际发生在这些文件描述符上时才向程序发送信号，固其性能可以根据发生IO事件的数量扩展，而与被检查的文件描述符数量无关，要想利用其优点，需执行如下两个步骤：

1. 通过fcntl的F_SETSIG操作指定一个实时信号，当文件描述符IO就绪时，此信号取代SIGIO被发送
2. 使用sigaction为上一步的实时信号安装信号处理器，并指定SIGINFO标记

使用F_SETSIG改变IO就绪发送默认信号SIGIO的理由：

1. 默认的SIGIO是标准的非排队信号，如有多个IO事件发送了信号，而SIGIO被阻塞了，解除阻塞后只能收到第一个信号
2. 指定SIGINFO标记可以标识出在哪个文件描述符上发生了事件，以及事件的类型

需要同时使用F_SETSIG以及SA_SIGINFO才能将一个合法的siginfo_t结构传递到信号处理器中，如果F_SETSIG时指定的sig为0，将回退到默认行为，siginfo_t与之相关的字段：

```
si_signo: 信号值
si_fd: 发生IO事件的文件描述符
si_code: 发生事件的类型代码
si_band: 位掩码，包含的位与poll返回的revents字段相同

si_code和si_band字段的可能值
si_code    si_band                             描述
POLL_IN   POLLIN | POLLRDNORM                 存在输入，文件结尾
POLL_OUT  POLLOUT | POLLWRNORM | POLLWRBAND   可输出
POLL_MSG  POLLIN | POLLRDNORM | POLLMSG       存在输出消息（不使用）
POLL_ERR  POLLERR                             IO错误
POLL_PRI  POLLPRI | POLLRDNORM                高优先级输入
POLL_HUP  POLLHUP | POLLERR                   宕机
```

纯输入驱动的程序中，可以进一步优化使用F_SETSIG，可以阻塞待发出的IO就绪信号，通过sigwaitinfo或sigtimedwait接收排队的信号，这种方式实际是以同步方式处理事件，同select和poll比，更为高效的获知文件描述符上发生的IO事件；但是实时信号的数量是有限的，为了防止溢出，一个设计良好的程序也应该为SIGIO安装信号处理器，一旦接收到了SIGIO信号，表明队列已满，可以先通过sigwaitinfo将队列中的实时信号全部获取，然后临时切换到select或poll，获取剩余的IO事件

非标准的fcntl操作F_SETOWN_EX和F_GETOWN_EX可以允许指定线程为IO就绪的目标，fcntl的第三个参数为：

```
struct f_owner_ex
{
  int type;
  pid_t pid;
}
type定义了pid的类型：
F_OWNER_PGRP：pid为进程组ID
F_OWNER_PID：pid为进程ID
F_OWNER_TID：pid为线程ID
```

### epoll编程接口

相比于select和poll，epoll的优点：

* 检查大量文件描述符时，epoll性能延展性比select和poll高很多
* select和poll只支持水平触发，信号驱动IO只支持边缘触发，epoll两者都支持

相比于信号驱动IO，epoll的优点：

* 避免复杂的信号处理流程，如信号队列溢出的处理
* 灵活性高，可以指定希望检查的事件类型

epoll API核心数据结构是epoll实例，与一个打开的文件描述符关联，这个文件描述符不是做IO操作，它是内核数据结构的句柄，实现了两个目的：

* 记录了在进程中声明过的感兴趣的文件描述符列表：兴趣列表
* 维护了处于IO就绪状态的文件描述符列表：就绪列表（兴趣列表的子集）

##### 创建epoll实例

```
#include <sys/epoll.h>

int epoll_create(int size);
// 返回值：若成功，返回文件描述符（epoll实例），若出错，返回-1
// size 表示想要通过epoll实例来检查的文件描述符个数，自内核2.6.8以来，已被忽略
// Linux专有文件/proc/sys/fd/epoll/max_user_watches表示每个用户可以注册到epoll实例上的文件描述符上限
```

##### 修改epoll兴趣列表

```
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *ev);
// 返回值：若成功，返回0，若出错，返回0
// epfd是epoll_create的返回值
// fd指明了要修改的兴趣列表的哪一个文件描述符，可以是管道、FIFO、套接字、POSIX消息队列、inotify实例、终端、设备或另一个epoll实例的文件描述符（可以为受检查的描述符建立一种层次关系），但是不能是普通文件或目录的文件描述符
op的取值：
EPOLL_CTL_ADD：将fd添加到epfd对应的兴趣列表中，已存在返回EEXIST错误
EPOLL_CTL_MOD：修改fd对应的事件，信息由ev传入，fd不在兴趣列表返回ENOENT错误
EPOLL_CTL_DEL：将fd从epfd对应的兴趣列表删除，fd不在兴趣列表返回ENOENT错误，关闭一个文件描述符自动将其从所有的epoll实例的兴趣列表删除

struct epoll_event
{
  uint32_t   events; // 位掩码
  epoll_data_t data; // 用户数据
}

typedef union epoll_data
{
  void  *ptr; // 用户实际数据
  int   fd;   // 文件描述符
  uint32 u32; // 
  uint64 u64; // 
}epoll_data_t;
```

##### 事件等待

```
int epoll_wait(int epfd, struct epoll_event *evlist, int maxevents, int timeout);
// 返回值：若成功，返回就绪列表的文件描述符个数，若超时返回0，若出错返回-1
// evlist返回就绪列表的文件描述符信息，空间由调用者申请，元素个数由maxevents指定
// 由于data字段是唯一可以获知这个事件相关的文件描述符的途径，调用epoll_ctl添加文件描述符时要么指定ev.data.fd为文件描述符，要么将ev.data.ptr设置为指向包含文件描述符的结构体
// timeout为-1表示一直阻塞，0表示执行一次非阻塞检查，大于0表示阻塞多少毫秒

位掩码        作为epoll_ctl的输入？ 由epoll_wait返回？      描述
EPOLLIN            Y                  Y          可读取非高优先级数据
EPOLLPRI           Y                  Y          可读取高优先级数据
EPOLLRDHUP         Y                  Y          套接字对端关闭
EPOLLOUT           Y                  Y          普通数据可写
EPOLLET            Y                             采用边缘触发事件通知
EPOLLONESHOT       Y                             完成事件通知后禁用检查
EPOLLERR                              Y          错误发生
EPOLLHUP                              Y          出现挂断
```

默认情况下，epoll_ctl添加EPOLL_CTL_ADD文件描述符到兴趣列表后，其一直保持激活状态，如果希望某个特定的文件描述符只得到一次通知，可以在ev.events中指定EPOLLONESHOT标记，要重新激活需要调用epoll_ctl的EPOLL_CTL_MOD操作

##### 深入探究epoll的语义

通过epoll_create创建epoll实例时，内核在内存中创建了一个新的i-node节点并打开文件描述，随后在调用进程中为打卡的这个文件描述分配一个新的文件描述符，与epoll的兴趣列表相关的是打开的文件描述，而不是epoll文件描述符，所谓的文件描述表示一个打开文件的上下文信息，由内核控制，相当于一个抽屉，而文件描述符相当于抽屉的把手

一旦所有指向打开的文件描述的文件描述符都被关闭后，这个打开的文件描述就从epoll兴趣列表中移除，如果通过dup或fork为打开的文件创建了描述符副本，那么这个打开的文件只会在原始的描述符以及所有其他副本都被关闭时才会移除

##### select/poll与epoll的性能对比

随着被监视的文件描述符数量的上升，select和poll的性能表现越来越差，而epoll的性能几乎不会降低，原因如下：

* 通过epoll_ctl指定监控的文件描述符时，内核会在打开的文件描述上下文关联的列表中记录该描述符，之后每次执行IO操作使得文件描述符成为就绪状态时，内核就在epoll描述符的就绪列表中添加一个元素，之后的epoll_wait调用从就绪列表中可以简单的取出这些元素
* 使用epoll_ctl在内核空间建立了一个数据结构，其会将待监视的文件描述符记录下来，一旦这个数据结构建立完成，每次调用epoll_wait时就不需要再传入任何与文件描述符有关的信息给内核了，而调用返回的信息中只包含那些已经处于就绪状态的描述符

##### 边沿触发通知

epoll默认是水平触发通知，表示epoll能在文件描述符上以非阻塞方式执行IO操作，同select和poll提供的通知方式相同，要以边缘触发通知，在epoll_ctl时在ev.events字段中指定EPOLLET标志，采用边缘触发通知机制的程序基本框架如下：

1. 让所有待监控的文件描述符和都是非阻塞
2. 通过epoll_ctl构建兴趣列表
3. 通过如下循环处理IO时间
   * 通过epoll_wait取得处于就绪状态的描述符列表
   * 针对每个就绪状态的文件描述符，不断进行IO处理直到相关系统调用（read、write、recv、send、accept）返回EAGAIN或EWOULDBLOCK错误

若有一个处于就绪状态的文件描述符上有大量的输入存在，以非阻塞方式读取时，其他的文件描述符可能处于饥饿状态（下次检查这些文件描述符是否处于就绪状态可能需要很长时间），该问题的一种处理方案是让程序维护一个列表，其中存放已经被通知为就绪状态的文件描述符，做如下循环：

1. 调用epoll_wait监视文件描述符，如果其已经注册到程序的维护列表中，那么这次监视操作的超时时间应该设为较小的值或者0，这样若无新的文件描述符成为就绪状态，程序可迅速进行到下一步去处理处于就绪状态的文件描述符了
2. 程序的维护列表中，只在已经注册为就绪状态的文件描述符上进行一定限度的IO操作（可能是以轮转调度的方式），当相关的非阻塞IO系统调用出现EAGAIN或EWOULDBLOCK错误时，将其从维护列表中移除

信号驱动IO采用的是边缘触发通知，故而也需要考虑文件描述符饥饿的情况

### 在信号和文件描述符上等待

有时候需要在一组文件描述符上等待IO就绪，也要等待信号，如果使用sigaction+select方式，在两个调用之间如果信号到来，则select依然会阻塞

```
#define _XOPEN_SOURCE 600
#include <sys/select.h>

int pselect(int nfds, fd_set *restrict readfds, fd_set *restrict writefds, fd_set *restrict errorfds, const struct timespec *restrict timeout, const sigset_t *restrict sigmask);
// 返回值：若成功返回就绪的文件描述符个数，若超时返回0，若出错返回-1
// SUSv3明确表示此调用不会修改timeout，若sigmask为NULL，那等同于select

ready=pselect(nfds, &readfds, &writefds, &errorfds, timeout, &sigmask);
如同以原子方式调用：
sigset_t origmask;
// 设置信号屏蔽字
sigprocmask(SIG_SETMASK, &sigmask, &origmask);
// 等待IO就绪状态
ready = select(nfds, &readfds, &writefds, &errorfds, timeout);
// 恢复信号屏蔽字
sigprocmask(SIG_SETMASK, &origmask, NULL);
```

Linux还提供了非标准的ppoll，同poll的关系类似于pselect和select，epoll_pwait是对epoll_wait的扩展

##### self-pipe技巧

由于pselect没有被广泛使用，可移植的程序必须采取其他手段避免当等待信号同时调用select时出现的竞态条件，通常会使用的技术为self-pipe

