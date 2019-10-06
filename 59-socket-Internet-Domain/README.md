### 第59章 SOCKET: Internet Domain

##### Internet Domain socket

Internet domain流socket基于TCP，提供可靠的双向的字节流通信信道；Internet domain数据报socket基于UDP，它与UNIX domain的数据报有差异：

* UNIX domain数据报socket是可靠的，但UDP则不可靠-数据报可能丢失、重复、到达顺序与发送顺序不一致
* UNIX domain数据报socket发送数据，在接收数据队列为满时阻塞，使用UDP接收队列满，数据报会被默默丢弃

##### 网络字节序

```
#include <arpa/inet.h>

uint32_t htonl(uint32_t hostint32); // 返回网络字节序表示的32位整数
uint16_t htons(uint16_t hostint16); // 返回网络字节序表示的16位整数
uint32_t ntohl(uint32_t netint32;   // 返回主机字节序表示的32位整数
uint16_t ntohs(uint16_t netint16);  // 返回主机字节序表示的16位整数

h: 主机host
n: 网络network
l: 长整型，4字节
s: 短整型，2字节
```

##### 数据表示

把数据变成一个标准格式以便在网络上传输的过程称为信号编集，存在多种标准，如XDR、ASN.1-BER、CORBA、XML等；更简单的方式是将所有传输的数据编码为文本形式，数据项之间使用特定字符如换行符分割，优点是可以使用telnet调试

##### Internet socket地址

```
IPv4：
struct int_addr
{
    in_addr_t s_addr;
};
struct socketaddr_in
{
    sa_family_t  sin_family: /* 地址协议族, AF_INET */
    in_port_t  sin_port;     /* 端口号 */
    struct int_addr  sin_addr; /* IPv4地址 */
    unsigned char sin_zero[8];  /* 填充字段，应该全部置为0 */
};

IPv6：
struct in6_addr
{
    uint8 s6_addr[16];
};
struct socketaddr_in6
{
    sa_family_t  sin6_family: /* 地址协议族, AF_INET6 */
    in_port_t  sin6_port;     /* 端口号 */
    struct in6_addr  sin6_addr; /* IPv6地址 */
    unsigned char sin_zero[8];  /* 填充字段，应该全部置为0 */
};

通用的结构，可以存储任意socket地址，包括IPv4和IPv6
struct sockaddr_storage
{
     sa_family_t ss_family;      /* 地址协议族 */
     __ss_aligntype __ss_align;  /* Force desired alignment.  */
     char __ss_padding[_SS_PADSIZE];
};
```

##### 主机和服务转换函数

二进制和人类可读的形式之间转换IPv4地址：

```
inet_aton()和inet_ntoa()：已经被废弃
```

二进制和人类可读的形式之间转换IPv4和IPv6地址：

```
inet_pton()和inet_ntop()：还可以处理IPV6地址
```

主机和服务名与二进制形式之间的转换：

```
gethostbyname()和getservbyname()：已经被废弃
getaddrinfo()：上面两个函数的现代继任者，将主机名和服务名转换为IP地址和端口号，可以透明的处理IPv4和IPv6地址
getnameinfo()：将一个IP地址和端口号转换为对应的主机名和服务名
```

