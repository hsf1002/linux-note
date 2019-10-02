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