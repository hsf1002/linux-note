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
getserverbyname()和getserverbyport()：已经被废弃
getaddrinfo()：上面函数的现代继任者，将主机名和服务名转换为IP地址和端口号，可以透明的处理IPv4和IPv6地址
getnameinfo()：将一个IP地址和端口号转换为对应的主机名和服务名
```

##### inet_pton()和inet_pton()

```
#include <arpa/inet.h>

将网络字节序的二进制地址转为文本字符串格式
const char * inet_ntop(int domain, const void *addrptr, char *strptr, size_t len); 
// 返回值：若成功，返回地址字符串指针，若出错，返回NULL
// domain只支持AF_INET和AF_INET6
// p表示presentation，n表示network

将strptr展现的字符串转为网络字节序的二进制IP地址
int inet_pton(int domain, const char *strptr, void *addrptr);
// 返回值：若成功，返回1，若格式无效，返回0，若出错，返回-1
```

##### 域名系统DNS

DNS解析请求分为递归和迭代，当调用getaddrinfo时，该函数会向本地DNS服务器发起一个递归请求，如果本地DNS服务器没有相关信息完成解析，那么它就迭代的解析这个域名

7个顶级域名：com、edu、net、org、int、mil、gov

##### /etc/services文件

该文件会记录服务名和端口号，getaddrinfo和getnameinfo会使用这个文件的信息在服务名和端口号之间进行转换，即使服务只使用了一种协议，IANA的策略是将两个端口号都分配给服务，在/etc/services中存在一个端口号并不能保证在实际环境中特定的服务器就能绑定到该端口上

```
3link           15363/udp   # 3Link Negotiation
3link           15363/tcp   # 3Link Negotiation
cisco-snat      15555/tcp   # Cisco Stateful NAT
cisco-snat      15555/udp   # Cisco Stateful NAT
ptp             15740/tcp   # Picture Transfer Protocol
ptp             15740/udp   # Picture Transfer Protocol
```

##### 独立于协议的主机和服务转换

给定一个主机名和服务名，getaddrinfo返回一个socket地址的结构列表，每个结构包含一个地址和端口号：

```
#include <netdb.h>

int getaddrinfo(const char *host, const char *service,
                       const struct addrinfo *hints,
                       struct addrinfo **result);
// 若成功，返回0，若出错，返回非0错误码 
// 可能需要向DNS服务器发送请求，可能要花费一段时间，getnameinfo也是类似
// 需要提供主机名和服务名，如果提供一个，另一个必须是空指针
// result返回的是一个结构列表，因为与host、service、hints指定的标准对应的主机和服务组合可能有多个
       
struct addrinfo 
{
       int              ai_flags;
       int              ai_family;
       int              ai_socktype;
       int              ai_protocol;
       socklen_t        ai_addrlen;
       struct sockaddr *ai_addr;
       char            *ai_canonname;
       struct addrinfo *ai_next;
};
hints.ai_flags是一个位掩码，会改变getaddrinfo的行为，由如下值的0个或多个OR得到：
AI_ADDRCONFIG: 在本地系统至少配置了一个IPv4/IPv6地址时返回IPv4/IPv6地址
AI_ALL: 见AI_V4MAPPED的描述
AI_CANONNAME: host不是NULL，返回一个包含主机的规范名的字符串
AI_NUMERICHOST: 将host解释为一个数值地址字符串
AI_NUMERICSERV: 将service解释为一个数值端口号
AI_PASSIVE: 返回一个监听socket的地址结构，此时host是NULL，result返回的socket地址结构的IP部分将包含一个通配IP地址（INADDR_ANY或IN6ADDR_ANY_INIT）；如果没有此标记，result返回的结构将能用于connect或sendto；如果host是NULL，返回的socket地址的IP部分将会被设置为回环IP地址（INADDR_LOOPBACK或IN6ADDR_LOOPBACK_INIT）
AI_V4MAPPED: 如果hints字段的ai_family指定了AF_INET6且没有找到匹配的IPv6地址，会在result返回的IPv4映射的IPv6地址结构
```

释放getaddrinfo动态为result分配的空间：

```
void freeaddrinfo(struct addrinfo *result);
```

如果getaddrinfo调用失败，不能使用perror或stderror生成错误信息，而要调用：

```
const char *gai_strerror(int error);
// 返回值：指向描述错误的字符串的指针
```

getaddrinfo的逆函数，将一个地址转换为一个主机名和一个服务名：

```
int getnameinfo(const struct sockaddr *addr, socklen_t addrlen, char *host, size_t 	  hostlen, char *serv, size_t servlen, int flags);
// 返回值：若成功返回0，若出错返回-1
// 如果不想获取主机名，host指定为NULL，hostlen指定为0，host和serv至少一个不能是NULL
// 套接字地址addr被翻译为一个主机名和一个服务名

flags是一个位掩码，控制着getnameinfo的行为：
NI_DGRAM: 默认getnameinfo返回与流socket（TCP）服务对应的名字，通常TCP与UDP端口对应的名字相同，在名字不同的场景下，该标记强制返回UDP对应的名字
NI_NAMEREQD: 默认如果无法解析主机名，会返回一个数值地址字符串，如果指定此标记，返回一个错误EAI_NONAME
NI_NUMERICHOST: 强制在host中返回一个数值地址字符串，在避免可能耗时较长的DNS服务时调用比较有用
NI_NUMEIRCSERV: 强制在service返回一个十进制端口号字符串，在避免不必要的搜索/etc/services的低效性时比较有用
```

##### Internet domain socket库

根据给定的socket type创建一个socket，并将其连接到通过host和service指定的地址：

```
int inetConnect(const char *host, const char *service, int type)
// 返回值：若成功，返回文件描述符，若出错，返回-1
```

创建一个监听流socket，该socket会绑定到由service指定的TCP端口的通配IP地址上：

```
int inetListen(const char *service, int backlog, socklen_t *addrlen)
// 返回值：若成功，返回文件描述符，若出错，返回-1
```

根据指定的type创建一个socket，并将其绑定到由service和type指定的端口的通配IP地址上：

```
int inetBind(const char *service, int type, socklen_t *addrlen)
// 返回值：若成功，返回文件描述符，若出错，返回-1
```

返回一个已null结尾的字符串，其包含了对应的主机名和端口号：

```
char *inetAddressStr(const struct sockaddr *addr, socklen_t addrlen, char *addrStr, int addrStrLen)
```

##### UNIX和Internet domain socket比较

Internet domain socket既可以运行在同一主机，又可以运行在不同主机上，通常是最简单的做法，然而UNIX domain socket存在的原有如下：

* 在一些实现上，UNIX domain socket速度更快
* 可以使用目录（Linux上是文件）权限控制对UNIX domain socket的访问，同时为验证客户端提供了最简单的方法
* 使用UNIX domain socket可以传递打开的文件描述符和发送者的验证信息

