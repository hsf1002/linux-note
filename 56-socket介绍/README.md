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

##### listen()：监听接入连接

```
int listen(int sockfd, int backlog)
// 返回值：若成功，返回0，若出错，返回-1
// 无法在一个已连接的socket上（已经成功执行connect的socket或从accept返回的socket）进行listen
// 要理解backlog，首先注意到客户端可能会在服务器调用accept之前调用connect，此时会产生一个未决的连接
// backlog提示系统该进程所要入队的未决的连接的请求数量，TCP最大值的默认值是128，一旦队列满，系统就会拒绝多余的连接请求，backlog的值应该基于服务器期望负载和处理量（接受连接请求与气动服务的数量）来选择
```

