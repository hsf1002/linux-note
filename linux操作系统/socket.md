### Socket通信之网络协议基本原理

![img](https://static001.geekbang.org/resource/image/f6/0e/f6982eb85dc66bd04200474efb3a050e.png)

 OSI 的标准七层模型和 TCP/IP 模型

![img](https://static001.geekbang.org/resource/image/92/0e/92f8e85f7b9a9f764c71081b56286e0e.png)

MAC 地址的定位功能局限在一个网络里面，即同一个网络号下的 IP 地址之间，可以通过 MAC 进行定位和通信。从 IP 地址获取 MAC 地址要通过 ARP 协议，是通过在本地发送广播包，也就是“吼”，获得的 MAC 地址。由于同一个网络内的机器数量有限，通过 MAC 地址的好处就是简单。匹配上 MAC 地址就接收，匹配不上就不接收，一旦跨网络通信，虽然 IP 地址保持不变，但是 MAC 地址每经过一个路由器就要换一次

Socket 属于操作系统的概念，而非网络协议分层的概念。只不过操作系统选择对于网络协议的实现模式是，二到四层的处理代码在内核里面，七层的处理代码让应用自己去做，两者需要跨内核态和用户态通信，就需要一个系统调用完成这个衔接，即Socket

##### 发送数据包

![img](https://static001.geekbang.org/resource/image/98/28/98a4496fff94eb02d1b1b8ae88f8dc28.jpeg)

1. 将请求封装为 HTTP 协议，通过 Socket 发送到内核。内核的网络协议栈里面，在 TCP 层创建用于维护连接、序列号、重传、拥塞控制的数据结构，将 HTTP 包加上 TCP 头，发送给 IP 层，IP 层加上 IP 头，发送给 MAC 层，MAC 层加上 MAC 头，从硬件网卡发出去
2. 网络包会先到达网络 1 的交换机。交换机为二层设备，这是因为，交换机只会处理到第二层，然后它会将网络包的 MAC 头拿下来，发现目标 MAC 是在自己右面的网口，于是就从这个网口发出去
3. 网络包会到达中间的 Linux 路由器，它左面的网卡会收到网络包，发现 MAC 地址匹配，就交给 IP 层，在 IP 层根据 IP 头中的信息，在路由表中查找。最终会从右面的网口发出去。路由器称为三层设备，因为它只会处理到第三层
4. 网络包会被转发到 Linux 服务器 B，它发现 MAC 地址匹配，就将 MAC 头取下来，交给上一层。IP 层发现 IP 地址匹配，将 IP 头取下来，交给上一层。TCP 层会根据 TCP 头中的序列号等信息，发现它是一个正确的网络包，就会将网络包缓存起来，等待应用层的读取
5. 应用层通过 Socket 监听某个端口，因而读取的时候，内核会根据 TCP 头中的端口号，将网络包发给相应的应用。HTTP 层的头和正文，是应用层来解析的。通过解析，应用层知道了客户端的请求，当应用层处理完 HTTP 的请求，会将结果仍然封装为 HTTP 的网络包，通过 Socket 接口，发送给内核
6. 内核会经过层层封装，从物理网口发送出去，经过网络 2 的交换机，Linux 路由器到达网络 1，经过网络 1 的交换机，到达 Linux 服务器 A。在 Linux 服务器 A 上，经过层层解封装，通过 socket 接口，根据客户端的随机端口号，发送给客户端的应用程序，浏览器。于是浏览器就能够显示出一个绚丽多彩的页面了

##### TCP socket过程

监听的 socket 和真正用来传送数据的 socket，是两个 socket，一个叫作监听 socket，一个叫作已连接 socket

![img](https://static001.geekbang.org/resource/image/99/da/997e39e5574252ada22220e4b3646dda.png)

1. 服务端和客户端都调用 socket，得到文件描述符
2. 服务端调用 listen，进行监听
3. 服务端调用 accept，等待客户端连接
4. 客户端调用 connect，连接服务端
5. 服务端 accept 返回用于传输的 socket 的文件描述符
6. 客户端调用 write 写入数据
7. 服务端调用 read 读取数据

##### UDP socket过程

![img](https://static001.geekbang.org/resource/image/28/b2/283b0e1c21f0277ba5b4b5cbcaca03b2.png)

UDP 没有连接，所以不需要三次握手，也不需要调用 listen 和 connect，但是 UDP 的交互仍然需要 IP 地址和端口号，因而也需要 bind。没有所谓的连接的发起方和接收方，甚至都不存在客户端和服务端的概念，都是客户端，也同时都是服务端。只要有一个 socket，多台机器就可以任意通信，不存在哪两台机器是属于一个连接的概念。每一个 UDP 的 socket 都需要 bind。每次通信时，调用 sendto 和 recvfrom，都要传入 IP 地址和端口

![img](https://static001.geekbang.org/resource/image/d3/5c/d34e667d1c3340deb8c82a2d44f2a65c.png)

##### 解析socket 函数

三个参数：

1. family：

```
#define AF_UNIX 1/* Unix domain sockets */
#define AF_INET 2/* Internet IP Protocol */
```

2. type：SOCK_STREAM 默认为IPPROTO_TCP、SOCK_DGRAM 默认为IPPROTO_UDP（IPPROTO_ICMP 也属于这种类型）、SOCK_RAW默认为IPPROTO_IP

3. ptotocal：

```
enum sock_type {
SOCK_STREAM = 1,
SOCK_DGRAM = 2,
SOCK_RAW = 3,
......
}
```

为了管理 family、type、protocol 这三个分类层次，内核会创建对应的数据结构。 sock_create->__sock_create

```
static struct inet_protosw inetsw_array[] =
{
  {
    .type =       SOCK_STREAM,
    .protocol =   IPPROTO_TCP,
    .prot =       &tcp_prot,
    .ops =        &inet_stream_ops,
    .flags =      INET_PROTOSW_PERMANENT |
            INET_PROTOSW_ICSK,
  },
  {
    .type =       SOCK_DGRAM,
    .protocol =   IPPROTO_UDP,
    .prot =       &udp_prot,
    .ops =        &inet_dgram_ops,
    .flags =      INET_PROTOSW_PERMANENT,
     },
     {
    .type =       SOCK_DGRAM,
    .protocol =   IPPROTO_ICMP,
    .prot =       &ping_prot,
    .ops =        &inet_sockraw_ops,
    .flags =      INET_PROTOSW_REUSE,
     },
     {
        .type =       SOCK_RAW,
      .protocol =   IPPROTO_IP,  /* wild card */
      .prot =       &raw_prot,
      .ops =        &inet_sockraw_ops,
      .flags =      INET_PROTOSW_REUSE,
     }
}
```

```
struct proto tcp_prot = {
  .name      = "TCP",
  .owner      = THIS_MODULE,
  .close      = tcp_close,
  .connect    = tcp_v4_connect,
  .disconnect    = tcp_disconnect,
  .accept      = inet_csk_accept,
  .ioctl      = tcp_ioctl,
  .init      = tcp_v4_init_sock,
  .destroy    = tcp_v4_destroy_sock,
  .shutdown    = tcp_shutdown,
  .setsockopt    = tcp_setsockopt,
  .getsockopt    = tcp_getsockopt,
  .keepalive    = tcp_set_keepalive,
  .recvmsg    = tcp_recvmsg,
  .sendmsg    = tcp_sendmsg,
  .sendpage    = tcp_sendpage,
  .backlog_rcv    = tcp_v4_do_rcv,
  .release_cb    = tcp_release_cb,
  .hash      = inet_hash,
    .get_port    = inet_csk_get_port,
......
}
```

##### 解析 bind 函数

sockfd_lookup_light 会根据 fd 文件描述符，找到 struct socket 结构。然后将 sockaddr 从用户态拷贝到内核态，然后调用 struct socket 结构里面 ops 的 bind 函数。根据前面创建 socket 的时候的设定，调用的是 inet_stream_ops 的 bind 函数，即 inet_bind。调用 sk_prot 的 get_port 函数，即 inet_csk_get_port 检查端口是否冲突，是否可以绑定。如果允许，则会设置 struct inet_sock 的本方的地址 inet_saddr 和本方的端口 inet_sport，对方的地址 inet_daddr 和对方的端口 inet_dport 都初始化为 0

##### 解析 listen 函数

1. sockfd_lookup_light会根据 fd 文件描述符，找到 struct socket 结构。接着调用 struct socket 结构里面 ops 的 listen 函数。根据前面创建 socket 的时候的设定，调用的是 inet_stream_ops 的 listen 函数，即 inet_listen
2. 如果这个 socket 还不在 TCP_LISTEN 状态，调用 inet_csk_listen_start 进入监听状态。这里面建立了一个新的结构 inet_connection_sock，它一开始是 struct inet_sock，inet_csk 做了一次强制类型转换，扩大了结构，包括处于各种状态的队列，各种超时时间、拥塞控制等字眼。TCP 是面向连接的，而客户端和服务端都是有一个结构维护连接的状态
3. 在 TCP 的状态里面，有一个 listen 状态，当调用 listen 函数之后，就会进入这个状态，虽然一般要等待服务端调用 accept 后，让客户端就发起连接。其实服务端一旦处于 listen 状态，不用 accept，客户端也能发起连接。其实 TCP 的状态中，没有一个是否被 accept 的状态
4. 在内核中，为每个 Socket 维护两个队列。一个是已经建立了连接的队列，这时候连接三次握手已经完毕，处于 established 状态；一个是还没有完全建立连接的队列，这个时候三次握手还没完成，处于 syn_rcvd 的状态。服务端调用 accept 函数，其实是在第一个队列中拿出一个已经完成的连接进行处理。如果还没有完成就阻塞等待。这里的 icsk_accept_queue 就是第一个队列
5. 初始化完之后，将 TCP 的状态设置为 TCP_LISTEN，再次调用 get_port 判断端口是否冲突。listen 的逻辑结束

##### 解析 accept 函数

1. 原来的 socket 是监听 socket，原来的 struct socket，基于它去创建一个新的 newsock。这才是连接 socket。除此之外，还会创建一个新的 struct file 和 fd，并关联到 socket
2. 调用 struct socket 的 sock->ops->accept，即 inet_stream_ops 的 accept 函数，即 inet_accept，inet_accept 会调用 struct sock 的 sk1->sk_prot->accept，即 tcp_prot 的 accept 函数，即inet_csk_accept 
3. 如果 icsk_accept_queue 为空，则调用 inet_csk_wait_for_connect 进行等待；等待的时候，调用 schedule_timeout，让出 CPU，并且将进程状态设置为 TASK_INTERRUPTIBLE
4. 如果再次 CPU 醒来，判断 icsk_accept_queue 是否为空，同时也会调用 signal_pending 看有没有信号可以处理。一旦 icsk_accept_queue 不为空，就从 inet_csk_wait_for_connect 中返回，在队列中取出一个 struct sock 对象赋值给 newsk

##### 解析 connect 函数

在三次握手结束后，icsk_accept_queue 才不为空

![img](https://static001.geekbang.org/resource/image/ab/df/ab92c2afb4aafb53143c471293ccb2df.png)

1. 通过 sockfd_lookup_light，根据 fd 文件描述符找到 struct socket 结构。调用 struct socket 结构里面 ops 的 connect 函数，根据前面创建 socket 的时候的设定，调用 inet_stream_ops 的 connect 函数，即 inet_stream_connect
2. 如果 socket 处于 SS_UNCONNECTED 状态，调用 struct sock 的 sk->sk_prot->connect，即 tcp_prot 的 connect 函数——tcp_v4_connect 函数
3. 在 tcp_v4_connect 函数中，ip_route_connect 做一个路由的选择，三次握手马上就要发送一个 SYN 包了，这就要凑齐源地址、源端口、目标地址、目标端口。目标地址和目标端口是服务端的，已经知道源端口是客户端随机分配的，源地址即哪一个网卡的 IP 地址
4. 在发送 SYN 之前，先将客户端 socket 的状态设置为 TCP_SYN_SENT。然后初始化 TCP 的 seq num，也即 write_seq，然后调用 tcp_connect 进行发送
5. 在 tcp_connect 中，有一个新的结构 struct tcp_sock，它是 struct inet_connection_sock 的一个扩展，其在 struct tcp_sock 开头的位置，通过强制类型转换访问，而struct tcp_sock 里面维护了更多的 TCP 的状态
6. tcp_init_nondata_skb 初始化一个 SYN 包，tcp_transmit_skb 将 SYN 包发送出去，inet_csk_reset_xmit_timer 设置了一个 timer，如果 SYN 发送不成功，则再次发送
7. __inet_stream_connect 函数，在调用 sk->sk_prot->connect 之后，inet_wait_for_connect 会一直等待客户端收到服务端的 ACK。而服务端在 accept 之后，也是在等待中
8. 通过 struct net_protocol 结构中的 handler 进行接收，调用的函数是 tcp_v4_rcv。接下来tcp_v4_rcv->tcp_v4_do_rcv->tcp_rcv_state_process。tcp_rcv_state_process，顾名思义，是用来处理接收一个网络包后引起状态变化的
9. 目前服务端是处于 TCP_LISTEN 状态的，而且发过来的包是 SYN，最终会调用 tcp_conn_request，里面调用了 send_synack->tcp_v4_send_synack。这是收到了 SYN 后，回复一个 SYN-ACK，回复完毕后，服务端处于 TCP_SYN_RECV
10. 此时轮到客户端接收网络包了。都是 TCP 协议栈，所以过程和服务端没有太多区别，还是会走到 tcp_rcv_state_process 函数，只不过由于客户端目前处于 TCP_SYN_SENT 状态，会调用 tcp_send_ack，发送一个 ACK-ACK，发送后客户端处于 TCP_ESTABLISHED 状态
11. 又轮到服务端接收网络包了，还是 tcp_rcv_state_process 函数处理。由于服务端目前处于状态 TCP_SYN_RECV 状态，当收到这个网络包的时候，服务端也处于 TCP_ESTABLISHED 状态，三次握手结束

![img](https://static001.geekbang.org/resource/image/c0/d8/c028381cf45d65d3f148e57408d26bd8.png)

Socket 系统调用有三级参数 family、type、protocal，通过这三级参数，分别在 net_proto_family 表中找到 type 链表，在 type 链表中找到 protocal 对应的操作。这个操作分为两层，对于 TCP 协议来讲，第一层是 inet_stream_ops 层，第二层是 tcp_prot 层

* bind 第一层调用 inet_stream_ops 的 inet_bind 函数，第二层调用 tcp_prot 的 inet_csk_get_port 函数
* listen 第一层调用 inet_stream_ops 的 inet_listen 函数，第二层调用 tcp_prot 的 inet_csk_get_port 函数
* accept 第一层调用 inet_stream_ops 的 inet_accept 函数，第二层调用 tcp_prot 的 inet_csk_accept 函数
* connect 第一层调用 inet_stream_ops 的 inet_stream_connect 函数，第二层调用 tcp_prot 的 tcp_v4_connect 函数