### 第56章 SOCKET介绍

##### 概述

现代操作系统至少支持以下domain：

* UNIX（AF_UNIX）：同一主机上的应用程序之间通信
* IPv4（AF_INET）：不同主机上的应用程序之间通过因特网协议第4版本进行通信
* IPv6（AF_INET6）：不同主机上的应用程序之间通过因特网协议第6版本进行通信

```
Domain    执行的通信        应用程序间的通信        地址格式            地址结构
AF_UNIX    内核中            同一主机              路径名             sockaddr_un
AF_INET    IPv4          通过IPv4连接的不同主机   32位IPv4+16位端口号   sockaddr_in
AF_INET6   IPv6          通过IPv6连接的不同主机   128位IPv6+16位端口号  sockaddr_in6
```

Internet domain中，数据报socket使用用户数据报协议UDP，流socket使用传输控制协议TCP

```
socket(): 创建一个新的socket
bind(): 通常服务器端需要调用将其绑定到一个众所周知的地址使得客户端能够定位到该socket
listen(): 允许一个流socket接收来自其他socket的接入
accept(): 监听流socket上的来自对等socket的连接
connect(): 建立与对等socket之间的连接
```

默认情况下，socket的系统调用如send、recv、sentto、recvfrom在IO无法立即完成时阻塞，可以通过fcntl的 F_SETFL操作启用O_NONBLOCK执行非阻塞

##### socket()：创建一个socket

```
#include <sys/socket.h>

int socket(int domain, int type, int protocol);
// 返回值：若成功，返回文件描述符，若出错，返回-1

type的类型：
SOCK_DGRAM: 固定长度的、无连接的、不可靠的报文传递
SOCK_RAW:   IP协议的数据报接口
SOCK_SEQPACKET: 固定长度的、有序的、可靠的、面向连接的报文传递
SOCK_STREAM: 有序的、可靠的、双向的、面向连接的字节流

protocol：
IPPROTO_IP: IPv4
IPPROTO_IPV6: IPv6
IPPROTO_ICMP: 因特网控制报文协议（Internet Control Message Protocol）
IPPROTO_RAW: 原始IP数据包协议
IPPROTO_TCP: 传输控制协议（Transmission Control Protocol）
IPPROTO_UDP: 用户数据报协议（User Datagram Protocol）

从内核2.6.27开始，为type提供了第二种用途，允许两个非标准的标记与sock类型取OR：
SOCK_CLOEXEC：导致内核为新文件描述符启动FD_CLOEXEC
SOCK_NONBLOCK：导致内核为新文件描述符启动O_NONBLOCK
```

##### bind()：将绑定到地址

```
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
// 返回值：若成功，返回0，若出错，返回-1

一般会将服务器的socket绑定到一个众所周知的地址，即一个固定的客户端提前就知道的地址，除了这种绑定之外，还有其他做法：对于Internet domain socket，可以不调用bind而直接调用listen，这导致内核为该socket选择一个临时端口，然后服务器通过getsockname获取socket地址，服务器需要向一个中心目录服务程序注册服务器的地址，之后客户端通过这个中心目录服务程序获取
```

##### struct sockaddr：通用socket地址结构

```
struct sockaddr
{
    sa_family_t  sa_family: /* 地址协议族 */
    char  sa_data[14]; /* 可变长度的地址 */
    ...
}
它的唯一用途是将各种domain的特定地址结构转换为单个类型提供给socket系统调用中的参数使用

Internet domain socket的地址结构：
struct socketaddr_in
{
    sa_family_t  sin_family: /* 地址协议族 */
    in_port_t  sin_port;  /* 端口号 */
    struct in6_addr  sin6_addr; /* IPv4地址 */
    unsigned char sin_zero[8];  /* 填充字段，应该全部置为0 */
}
```

##### 流socket

1. 首先双方都需要通过socket()创建，如同安装电话
2. 双方进行连接
   * 服务端调用bind将socket绑定到一个众所周知的地址，再调用listen通知内核它接受接入连接的意愿
   * 客户端通过connect建立连接，同时需要指定需要连接的socket地址，类似拨打号码
   * 服务端调用accept接受连接，如果在connect前调用了accept，就会阻塞
3. 通过传统的read/write或socket特有的send/recv进行通信，直到一方调用close关闭连接

![SOCKET](https://images2015.cnblogs.com/blog/700495/201611/700495-20161122153736846-1965205030.png)

##### listen()：监听接入连接

```
int listen(int sockfd, int backlog)
// 返回值：若成功，返回0，若出错，返回-1
// 无法在一个已连接的socket上（已经成功执行connect的socket或从accept返回的socket）进行listen
// 要理解backlog，首先注意到客户端可能会在服务器调用accept之前调用connect，此时会产生一个未决的连接
// backlog提示系统该进程所要入队的未决的连接的请求数量，TCP最大值的默认值是128，一旦队列满，系统就会拒绝多余的连接请求，backlog的值应该基于服务器期望负载和处理量（接受连接请求与气动服务的数量）来选择
```

##### accept(): 接受连接

```
int accept(int sockfd,struct sockaddr *addr,socklen_t *addrlen);
// 返回值：若成功，返回0，若出错，返回-1，如果不关心客户端的地址，addr和addrlen置为NULL，后续可通过getpeername来获取对端地址
// 如果套接字描述符处于非阻塞模式，就会返回-1，且将errno设置为EAGAIN或EWOULDBLOCK
// 如果服务器调用accept，且当前没有连接请求，会阻塞直到一个请求到来，服务器可以使用poll或select等待一个请求的到来
// 关键是accept会创建一个新的socket，正是这个新的socket会和执行connect的客户端进行连接
// 从内核2.6.28开始，Linux支持新的非标准系统调用accept4()，新增一个额外的flag，支持两个标记：SOCK_CLOEXEC和SOCK_NONBLOCK
```

##### connect(): 连接到对等socket

```
int connect(int sockfd, const struct sockaddr* addr, socklen_t len);
// 返回值：若成功，返回0，若出错，返回-1
// 如果套接字描述符处于非阻塞模式，连接不能马上建立，就会返回-1，且将errno设置为EINPROGRESS，可以使用poll或select判断文件描述符何时可写，如果可写，连接完成
// 如果sockfd没有绑定到一个地址，connect会给调用者绑定一个默认地址
// 基于BSD的套接字实现中，如果第一次连接失败，那么在TCP中继续使用同一个套接字描述符，仍然会失败，程序应该处理关闭套接字，如果需要重试，必须打开一个新的套接字
// connect可以用于无连接的网络服务，SOCK_DGRAM，这样传输的报文的目标地址会设置成connect调用中指定的地址，每次传输时不需要再提供地址
```

##### close(): 连接终止

终止一个连接的常见方式是调用close，如果多个文件描述符引用一个socket，所有描述符关闭后连接就会终止

##### shutdown()：连接终止

```
nt shutdown(int sockfd, int how);
// 若成功，返回0，若出错，返回-1

close可以关闭一个套接字，但是只有最后一个活动引用关闭时，close才会释放网络端点，而shutdown允许一个套接字处于不活动状态
```

 ##### 数据报socket

1. 首先双方都需要通过socket()创建，如同邮局收发信件
2. 服务端调用bind将socket绑定到一个众所周知的地址，再调用listen通知内核它接受接入连接的意愿
3. 通过sendto发送数据，同时需要指定需要发送的socket地址
4. 通过recvfrom接收数据，没有数据时阻塞
5. 调用close关闭socket

![img](http://images2015.cnblogs.com/blog/1019006/201703/1019006-20170303103250782-1966231336.png)

##### sendto()和recvfrom()：交换数据报

```
ssize_t sendto(int sock, const void *buf, size_t len, int flags, const struct sockaddr *to, socklen_t tolen);
// 若成功，返回发送的字节数，若出错，返回-1
// 对于面向连接的套接字，目标地址被忽略，因为连接中隐含了地址，对于无连接的套接字，除非先通过connect设置了目标地址，否则不能使用send。sendto提供了发送报文的另一种方案

int recvfrom(int sockfd, void *buf, size_t len，int flags, struct sockaddr *addr, socklen_t *addrlen); 
// 若成功，返回数据的字节数，若无可用数据或对方已经结束，返回0，若出错，返回-1
// 如果addr非空，将包含数据发送者的套接字端点地址
// 通常用于无连接的套接字，否则，addr和addrlen置为NULL，等同于recv
```

##### 数据报上使用connect()

数据报上调用connect会导致内核记录这个socket的对等socket的地址，当一个数据报已经连接后：

* 数据报的发送可以使用write或send
* 在这个socket上只能读取对等socket发送的数据报

connect的作用对数据报socket是不对等的，上面论断适用于调用了connect的数据报socket，并不适用于远程的对等的socket，除非它也调用了connect