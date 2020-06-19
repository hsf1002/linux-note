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

### 发送网络包

* VFS 层：write 系统调用找到 struct file，根据里面的 file_operations 的定义，调用 sock_write_iter 函数。sock_write_iter 函数调用 sock_sendmsg 函数
* Socket 层：从 struct file 里面的 private_data 得到 struct socket，根据里面 ops 的定义，调用 inet_sendmsg 函数
* Sock 层：从 struct socket 里面的 sk 得到 struct sock，根据里面 sk_prot 的定义，调用 tcp_sendmsg 函数
* TCP 层：tcp_sendmsg 函数会调用 tcp_write_xmit 函数，tcp_write_xmit 函数会调用 tcp_transmit_skb，在这里实现了 TCP 层面向连接的逻辑
* IP 层：扩展 struct sock，得到 struct inet_connection_sock，根据里面 icsk_af_ops 的定义，调用 ip_queue_xmit 函数
* IP 层：ip_route_output_ports 函数里面会调用 fib_lookup 查找路由表
* 在 IP 层里面要做的另一个事情是填写 IP 层的头
* 在 IP 层还要做的一件事情就是通过 iptables 规则
* MAC 层：IP 层调用 ip_finish_output 进行 MAC 层
* MAC 层需要 ARP 获得 MAC 地址，因而要调用` ___neigh_lookup_noref` 查找属于同一个网段的邻居，他会调用 neigh_probe 发送 ARP
* 有了 MAC 地址，就可以调用 dev_queue_xmit 发送二层网络包了，它会调用 `__dev_xmit_skb` 会将请求放入队列
* 设备层：网络包的发送会触发一个软中断 NET_TX_SOFTIRQ 来处理队列中的数据。软中断的处理函数是 net_tx_action
* 在软中断处理函数中，会将网络包从队列上拿下来，调用网络设备的传输函数 ixgb_xmit_frame，将网络包发到设备的队列上去

![img](https://static001.geekbang.org/resource/image/79/6f/79cc42f3163d159a66e163c006d9f36f.png)

##### 解析 socket 的 Write

对于 socket ，它的 file_operations 定义如下：

```
static const struct file_operations socket_file_ops = {
  .owner =  THIS_MODULE,
  .llseek =  no_llseek,
  .read_iter =  sock_read_iter,
  .write_iter =  sock_write_iter,
  .poll =    sock_poll,
  .unlocked_ioctl = sock_ioctl,
  .mmap =    sock_mmap,
  .release =  sock_close,
  .fasync =  sock_fasync,
  .sendpage =  sock_sendpage,
  .splice_write = generic_splice_sendpage,
  .splice_read =  sock_splice_read,
};
```

在 sock_write_iter 中，通过 VFS 中的 struct file，将创建好的 socket 结构拿出来，然后调用 sock_sendmsg->sock_sendmsg_nosec->sendmsg，根据 inet_stream_ops 的定义，调用是inet_sendmsg，从 socket 结构中，可以得到更底层的 sock 结构，然后调用 sk_prot 的 sendmsg

##### 解析 tcp_sendmsg

根据 tcp_prot 的定义，实际调用的是 tcp_sendmsg，在内核协议栈里面，网络包的数据都是由 struct sk_buff 维护的，第一件事情就是找到一个空闲的内存空间，将用户要写入的数据，拷贝到 struct sk_buff 的管辖范围内。第二件事情就是发送 struct sk_buff

MTU（Maximum Transmission Unit，最大传输单元）：以以太网为例，为 1500 个 Byte，前面有 6 个 Byte 的目标 MAC 地址，6 个 Byte 的源 MAC 地址，2 个 Byte 的类型，后面有 4 个 Byte 的 CRC 校验，共 1518 个 Byte。在 IP 层，一个 IP 数据报在以太网中传输，如果它的长度大于该 MTU 值，就要进行分片传输。在 TCP 层有个 MSS（Maximum Segment Size，最大分段大小），等于 MTU 减去 IP 头，再减去 TCP 头。即在不分片的情况下，TCP 里面放的最大内容

struct sk_buff 是存储网络包的重要的数据结构，在应用层数据包叫 data，在 TCP 层称为 segment，在 IP 层叫 packet，在数据链路层称为 frame。它是一个链表，将 struct sk_buff 结构串起来。里面有二层的 mac_header、三层的 network_header 和四层的 transport_header，head 指向分配的内存块起始地址。data 这个指针指向的位置是可变的。它有可能随着报文所处的层次而变动。当接收报文时，从网卡驱动开始，通过协议栈层层往上传送数据报，通过增加 skb->data 的值，来逐步剥离协议首部。而要发送报文时，各协议会创建 sk_buff{}，在经过各下层协议时，通过减少 skb->data 的值来增加协议首部。tail 指向数据的结尾，end 指向分配的内存块的结束地址。要分配的 sk_stream_alloc_skb 会最终调用到 __alloc_skb。它除了分配一个 sk_buff 结构之外，还要分配 sk_buff 指向的数据区域。这段数据区域分为：第一部分是连续的数据区域。第二部分是一个 struct skb_shared_info 结构，如果TCP层的数据超过了一个 IP 包的承载能力，按照 MSS 的定义，拆分成一个个的 Segment 放在一个个的 IP 包里面，一次写一点，这样数据是分散的，在 IP 层还要通过内存拷贝合成一个 IP 包。为了减少内存拷贝的代价，有的网络设备支持分散聚合（Scatter/Gather）I/O

![img](https://static001.geekbang.org/resource/image/9a/b8/9ad34c3c748978f915027d5085a858b8.png)

 skb_add_data_nocache 将数据拷贝到连续的数据区域。skb_copy_to_page_nocache 将数据拷贝到不需要连续的页面区域。如果积累的数据报数目太多了，调用 __tcp_push_pending_frames 发送网络包。如果是第一个网络包，需要马上发送，调用 tcp_push_one。最终都会调用 tcp_write_xmit 发送网络包

##### 解析 tcp_write_xmit 

里面涉及到很多传输算法，如：

*  TSO（TCP Segmentation Offload）：如果发送的网络包非常大，要进行分段。可以由协议栈代码在内核做，但是费 CPU，另一种方式是延迟到硬件网卡去做，需要网卡支持对大数据包进行自动分段，可以降低 CPU 负载
* 拥塞窗口（cwnd，congestion window）：为了避免拼命发包，把网络塞满了，定义一个窗口的概念，在这个窗口之内的才能发送，超过这个窗口的就不能发送，来控制发送的频率，如下：开始的窗口只有一个 mss 大小叫作 slow start（慢启动）。增长速度很快，翻倍增长。一旦到达一个临界值 ssthresh，就变成线性增长，叫做拥塞避免。一旦出现丢包，就是真正拥塞，一种方法是马上降回到一个 mss，然后重复先翻倍再线性对的过程。但是太过激进，第二种方法，是降到当前 cwnd 的一半，然后进行线性增长（红线）

![img](https://static001.geekbang.org/resource/image/40/1f/404a6c5041452c0641ae3cba5319dc1f.png)

* 接收窗口rwnd（receive window）：也叫滑动窗口。如果说拥塞窗口是为了怕把网络塞满，在出现丢包的时候减少发送速度，那么滑动窗口就是为了怕把接收方塞满，而控制发送速度。滑动窗口，其实就是接收方告诉发送方自己的网络包的接收能力，超过这个能力就无法接收了。因为滑动窗口的存在，将发送方的缓存分成了四个部分：

  * 第一部分：发送了并且已经确认的。这部分是已经发送完毕的网络包，没有用了，可以回收
  * 第二部分：发送了但尚未确认的。这部分发送方要等待，万一发送不成功，还要重新发送，所以不能删除
  * 第三部分：没有发送但是已经等待发送。这部分是接收方空闲的能力，可以马上发送，接收方收得了
  * 第四部分：没有发送，并且暂时还不会发送的。这部分超过了接收方的接收能力，再发送接收方就收不

  ![img](https://static001.geekbang.org/resource/image/97/65/9791e2f9ff63a9d8f849df7cd55fe965.png)

  因为滑动窗口的存在，接收方的缓存也分成了三个部分：

  * 第一部分：接受并且确认过的任务。这部分完全接收成功了，可以交给应用层了
  * 第二部分：还没接收，但是马上就能接收的任务。这部分有的网络包到达了，但是还没确认，不算完全完毕，有的还没有到达，那就是接收方能够接受的最大的网络包数量
  * 第三部分：还没接收，也没法接收的任务。这部分已经超出接收方能力

  在网络包的交互过程中，接收方会将第二部分的大小，作为 AdvertisedWindow 发送给发送方，发送方就可以根据它来调整发送速度了

  ![img](https://static001.geekbang.org/resource/image/b6/31/b62eea403e665bb196dceba571392531.png)

最后调用 tcp_transmit_skb，真的去发送一个网络包

![img](https://static001.geekbang.org/resource/image/be/0e/be225a97816a664367f29be9046aa30e.png)

填充 TCP 头：源端口，设置为 inet_sport，目标端口，设置为 inet_dport；序列号，设置为 tcb->seq；确认序列号，设置为 tp->rcv_nxt。把所有的 flags 设置为 tcb->tcp_flags。设置选项为 opts。设置窗口大小为 tp->rcv_wnd。全部设置完毕之后，就会调用 icsk_af_ops 的 queue_xmit 方法，icsk_af_ops 指向 ipv4_specific，即调用的是 ip_queue_xmit 函数

```
const struct inet_connection_sock_af_ops ipv4_specific = {
        .queue_xmit        = ip_queue_xmit,
        .send_check        = tcp_v4_send_check,
        .rebuild_header    = inet_sk_rebuild_header,
        .sk_rx_dst_set     = inet_sk_rx_dst_set,
        .conn_request      = tcp_v4_conn_request,
        .syn_recv_sock     = tcp_v4_syn_recv_sock,
        .net_header_len    = sizeof(struct iphdr),
        .setsockopt        = ip_setsockopt,
        .getsockopt        = ip_getsockopt,
        .addr2sockaddr     = inet_csk_addr2sockaddr,
        .sockaddr_len      = sizeof(struct sockaddr_in),
        .mtu_reduced       = tcp_v4_mtu_reduced,
};
```

##### 解析 ip_queue_xmit 

* 第一部分，选取路由，应该从哪个网卡出去。主要由 ip_route_output_ports 函数完成。最终是ip_route_output_key_hash_rcu。其先会调用 fib_lookup，FIB 全称是 Forwarding Information Base，转发信息表，即路由表，可以有多个，一般会有一个主表，RT_TABLE_MAIN。然后 fib_table_lookup 函数在这个表里面进行查找，最后调用 __mkroute_output，创建一个 struct rtable，表示找到的路由表项

![img](https://static001.geekbang.org/resource/image/f6/0e/f6982eb85dc66bd04200474efb3a050e.png)

* 第二部分，准备 IP 层的头，往里面填充内容。服务类型设置为 tos，标识位里面设置是否允许分片 frag_off,如果不允许，而遇到 MTU 太小过不去的情况，就发送 ICMP 报错。TTL 是这个包的存活时间，为了防止一个 IP 包迷路以后一直存活下去，每经过一个路由器 TTL 都减一，减为零则“死去”。设置 protocol，指的是更上层的协议，这里是 TCP。源地址和目标地址由 ip_copy_addrs 设置。最后设置 options

![img](https://static001.geekbang.org/resource/image/6b/2b/6b2ea7148a8e04138a2228c5dbc7182b.png)

* 第三部分，调用 ip_local_out 发送 IP 包，里面调用了 nf_hook。nf 的意思是 Netfilter，这是 Linux 内核的一个机制，用于在网络发送和转发的关键节点上加上 hook 函数，这些函数可以截获数据包，对数据包进行干预。一个著名的实现，是内核模块 ip_tables。在用户态，还有一个客户端程序 iptables，用命令行来干预内核的规则

  ![img](https://static001.geekbang.org/resource/image/75/4d/75c8257049eed99499e802fcc2eacf4d.png)

  iptables 有表和链的概念，最重要的是两个表：

  * filter 表处理过滤功能，主要包含以下三个链
    * INPUT 链：过滤所有目标地址是本机的数据包
    * FORWARD 链：过滤所有路过本机的数据包
    * OUTPUT 链：过滤所有由本机产生的数据包

  * nat 表处理网络地址转换，可以进行 SNAT（改变源地址）、DNAT（改变目标地址），包含以下三个链
    * PREROUTING 链：可以在数据包到达时改变目标地址
    * OUTPUT 链：可以改变本地产生的数据包的目标地址
    * POSTROUTING 链：在数据包离开时改变数据包的源地址

![img](https://static001.geekbang.org/resource/image/76/da/765e5431fe4b17f62b1b5712cc82abda.png)

ip_local_out 再调用 dst_output，真正的发送数据，最终调用调用 ip_finish_output

##### 解析 ip_finish_output

在 ip_finish_output2 中，先找到 struct rtable 路由表里面的下一跳，下一跳一定和本机在同一个局域网中，可以通过二层进行通信，因而通过 `__ipv4_neigh_lookup_noref`，查找如何通过二层访问下一跳。`__ipv4_neigh_lookup_noref`是从本地的 ARP 表中查找下一跳的 MAC 地址。ARP 表的定义如下：

```
struct neigh_table arp_tbl = {
    .family     = AF_INET,
    .key_len    = 4,    
    .protocol   = cpu_to_be16(ETH_P_IP),
    .hash       = arp_hash,
    .key_eq     = arp_key_eq,
    .constructor    = arp_constructor,
    .proxy_redo = parp_redo,
    .id     = "arp_cache",
......
    .gc_interval    = 30 * HZ, 
    .gc_thresh1 = 128,  
    .gc_thresh2 = 512,  
    .gc_thresh3 = 1024,
};
```

如果在 ARP 表中没有找到相应的项，则调用 `__neigh_create` 进行创建，先调用 neigh_alloc创建一个 struct neighbour 结构，用于维护 MAC 地址和 ARP 相关的信息。大家都在一个局域网里面，可以通过 MAC 地址访问到，最后是将创建的 struct neighbour 结构放入一个哈希表，这是一个数组加链表的链式哈希表，先计算出哈希值 hash_val，得到相应的链表，然后循环这个链表找到对应的项，如果找不到就在最后插入一项。回到 ip_finish_output2，在 `__neigh_create` 之后，会调用 neigh_output 发送网络包。调用arp_send_dst，创建并发送一个 arp 包，接着调用 dev_queue_xmit 发送二层网络包

qdisc 全称是 queueing discipline，即排队规则。内核如果需要通过某个网络接口发送数据包，都需要按照为这个接口配置的 qdisc（排队规则）把数据包加入队列。最简单的 qdisc 是 pfifo，它不对进入的数据包做任何的处理，数据包采用先入先出的方式通过队列。pfifo_fast 稍微复杂一些，它的队列包括三个波段（band）。在每个波段里面，使用先进先出规则。三个波段的优先级也不相同。band 0 的优先级最高，band 2 的最低。如果 band 0 里面有数据包，系统就不会处理 band 1 里面的数据包，band 1 和 band 2 之间也是一样。数据包是按照服务类型（Type of Service，TOS）被分配到三个波段里面的。TOS 是 IP 头里面的一个字段，代表了当前的包是高优先级的，还是低优先级的。pfifo_fast 分为三个先入先出的队列，称为三个 Band。根据网络包里面的 TOS，看这个包到底应该进入哪个队列。TOS 总共四位，每一位表示的意思不同，总共十六种类型

![img](https://static001.geekbang.org/resource/image/ab/d9/ab6af2f9e1a64868636080a05cfde0d9.png)

`__dev_xmit_skb` 开始进行网络包发送，它首先会将请求放入队列，然后调用 `__qdisc_run` 处理队列中的数据，最终qdisc_restart 将网络包从 Qdisc 的队列中拿下来，然后调用 sch_direct_xmit 进行发送。在 dev_hard_start_xmit 中，是一个 while 循环。每次在队列中取出一个 sk_buff，调用 xmit_one 发送。接下来的调用链为：xmit_one->netdev_start_xmit->__netdev_start_xmit，这个时候，已经到了设备驱动层了，drivers/net/ethernet/intel/ixgb/ixgb_main.c 里面有对于这个网卡的操作的定义

```
static const struct net_device_ops ixgb_netdev_ops = {
        .ndo_open               = ixgb_open,
        .ndo_stop               = ixgb_close,
        .ndo_start_xmit         = ixgb_xmit_frame,
        .ndo_set_rx_mode        = ixgb_set_multi,
        .ndo_validate_addr      = eth_validate_addr,
        .ndo_set_mac_address    = ixgb_set_mac,
        .ndo_change_mtu         = ixgb_change_mtu,
        .ndo_tx_timeout         = ixgb_tx_timeout,
        .ndo_vlan_rx_add_vid    = ixgb_vlan_rx_add_vid,
        .ndo_vlan_rx_kill_vid   = ixgb_vlan_rx_kill_vid,
        .ndo_fix_features       = ixgb_fix_features,
        .ndo_set_features       = ixgb_set_features,
};
```

在 ixgb_xmit_frame 中，得到这个网卡对应的适配器，然后将其放入硬件网卡的队列中。至此，整个发送才算结束

### 接收网络包

![img](https://static001.geekbang.org/resource/image/20/52/20df32a842495d0f629ca5da53e47152.png)

* 硬件网卡接收到网络包之后，通过 DMA 技术，将网络包放入 Ring Buffer
* 硬件网卡通过中断通知 CPU 新的网络包的到来
* 网卡驱动程序会注册中断处理函数 ixgb_intr。中断处理函数处理完需要暂时屏蔽中断的核心流程之后，通过软中断 NET_RX_SOFTIRQ 触发接下来的处理过程
* NET_RX_SOFTIRQ 软中断处理函数 net_rx_action，net_rx_action 会调用 napi_poll，进而调用 ixgb_clean_rx_irq，从 Ring Buffer 中读取数据到内核 struct sk_buff
* 调用 netif_receive_skb 进入内核网络协议栈，进行一些关于 VLAN 的二层逻辑处理后，调用 ip_rcv 进入三层 IP 层
* 在 IP 层，处理 iptables 规则，然后调用 ip_local_deliver，交给更上层 TCP 层
* 在 TCP 层调用 tcp_v4_rcv
* 在 tcp_v4_do_rcv 中，如果是处于 TCP_ESTABLISHED 状态，调用 tcp_rcv_established，其他的状态，调用 tcp_rcv_state_process
* 在 tcp_rcv_established 中，调用 tcp_data_queue，如果序列号能够接的上，则放入 sk_receive_queue 队列；如果序列号接不上，则暂时放入 out_of_order_queue 队列，等序列号能够接上的时候，再放入 sk_receive_queue 队列

接下来就是用户态读取网络包的过程，这个过程分成几个层次

* VFS 层：read 系统调用找到 struct file，根据里面的 file_operations 的定义，调用 sock_read_iter 函数。sock_read_iter 函数调用 sock_recvmsg 函数
* Socket 层：从 struct file 里面的 private_data 得到 struct socket，根据里面 ops 的定义，调用 inet_recvmsg 函数
* Sock 层：从 struct socket 里面的 sk 得到 struct sock，根据里面 sk_prot 的定义，调用 tcp_recvmsg 函数
* TCP 层：tcp_recvmsg 函数会依次读取 receive_queue 队列、prequeue 队列和 backlog 队列

##### 设备驱动层

NAPI：当一些网络包到来触发了中断，内核处理完这些网络包之后，先进入主动轮询 poll 网卡的方式，主动去接收到来的网络包。如果一直有，就一直处理，等处理告一段落，就返回干其他的事情。当再有下一批网络包到来的时候，再中断，再轮询 poll。这样就会大大减少中断的数量，提升网络处理的效率

以drivers/net/ethernet/intel/ixgb/ixgb_main.c 为例，在网卡驱动程序初始化的时候，调用 ixgb_init_module，注册一个驱动 ixgb_driver，并且调用它的 probe 函数 ixgb_probe，创建一个 struct net_device 表示这个网络设备，并且 netif_napi_add 函数为这个网络设备注册一个轮询 poll 函数 ixgb_clean，将来一旦出现网络包的时候，就是要通过它来轮询。当一个网卡被激活的时候，调用函数 ixgb_open->ixgb_up，在里面注册一个硬件的中断处理函数。如果一个网络包到来，触发了硬件中断，就会调用 ixgb_intr，里面会调用 __napi_schedule。它会触发一个软中断 NET_RX_SOFTIRQ，通过软中断触发中断处理的延迟处理部分。软中断 NET_RX_SOFTIRQ 对应的中断处理函数是 net_rx_action。其中会得到 struct softnet_data 结构，output_queue 用于网络包的发送，poll_list 用于网络包的接收

```
struct softnet_data {
  struct list_head  poll_list;
......
  struct Qdisc    *output_queue;
  struct Qdisc    **output_queue_tailp;
......
}
```

在 net_rx_action是一个循环，在 poll_list 里面取出网络包到达的设备，然后调用 napi_poll 来轮询这些设备，napi_poll 会调用最初设备初始化的时候，注册的 poll 函数，对于 ixgb_driver，对应的函数是 ixgb_clean->ixgb_clean_rx_irq。有一个用于接收网络包的 rx_ring。它是一个环，从网卡硬件接收的包会放在这个环里面。这个环里面的 buffer_info[]是一个数组，存放的是网络包的内容，ixgb_check_copybreak 函数将 buffer_info 里面的内容，拷贝到 struct sk_buff *skb，从而可以作为一个网络包进行后续的处理，然后调用 netif_receive_skb

##### 网络协议栈的二层逻辑

`netif_receive_skb->__netif_receive_skb_core`，deliver_ptype_list_skb 在一个协议列表中逐个匹配。如果能够匹配到，就返回。这些协议的注册在网络协议栈初始化的时候， inet_init 函数调用 dev_add_pack(&ip_packet_type)，添加 IP 协议。协议被放在一个链表里面，假设这个时候的网络包是一个 IP 包，则在这个链表里面一定能够找到 ip_packet_type，在 `__netif_receive_skb_core` 中会调用 ip_packet_type 的 func 函数。接下来，ip_rcv 会被调用

```
static struct packet_type ip_packet_type __read_mostly = {
  .type = cpu_to_be16(ETH_P_IP),
  .func = ip_rcv,
};
```

##### 网络协议栈的 IP 层

处理逻辑就从二层到了三层，IP 层，在ip_rcv中得到 IP 头，第一个 hook 点是 NF_INET_PRE_ROUTING，就是 iptables 的 PREROUTING 链。如果里面有规则，则执行规则，然后调用 ip_rcv_finish，得到网络包对应的路由表，然后调用 dst_input，其实是 struct rtable 的成员的 dst 的 input 函数。在 rt_dst_alloc 中，input 函数指向的是 ip_local_deliver。如果 IP 层进行了分段，则进行重新的组合。hook 点在 NF_INET_LOCAL_IN，对应 iptables 里面的 INPUT 链。在经过 iptables 规则处理完毕后，调用 ip_local_deliver_finish。在 IP 头中，有一个字段 protocol 用于指定里面一层的协议，此处是 TCP 协议。从 inet_protos 数组中，找出 TCP 协议对应的处理函数。里面的内容是 struct net_protocol。在系统初始化的时候，网络协议栈的初始化调用的是 inet_init，它会调用 inet_add_protocol，将 TCP 协议对应的处理函数 tcp_protocol、UDP 协议对应的处理函数 udp_protocol，放到 inet_protos 数组中。在上面的网络包的接收过程中，会取出 TCP 协议对应的处理函数 tcp_protocol，然后调用 handler 函数，即 tcp_v4_rcv 函数

##### 网络协议栈的 TCP 层

从 tcp_v4_rcv 函数开始，就从 IP 层到了 TCP 层。得到 TCP 的头之后，开始处理 TCP 层的事情。TCP状态被维护在数据结构 struct sock 里面，根据 IP 地址以及 TCP 头里面的内容，在 tcp_hashinfo 中找到这个包对应的 struct sock，从而得到这个包对应的连接的状态，再根据不同的状态做不同的处理。最主流的网络包的接收过程，涉及三个队列：backlog 队列、prequeue 队列、sk_receive_queue 队列，由于同一个网络包要在三个主体之间交接：

* 第一个主体是软中断的处理过程。在执行 tcp_v4_rcv 函数的时候，依然处于软中断的处理逻辑里，所以必然会占用这个软中断
* 第二个主体是用户态进程。如果用户态触发系统调用 read 读取网络包，也要从队列里面找
* 第三个主体是内核协议栈。哪怕用户进程没有调用 read读取网络包，当网络包来的时候，也得接收

1. 如果没有一个用户态进程等着读数据，内核协议栈调用 tcp_add_backlog，暂存在 backlog 队列中，并且离开软中断的处理过程

2. 如果有一个用户态进程等待读取数据，调用 tcp_prequeue，赶紧放入 prequeue 队列，并且离开软中断的处理过程

3. 在这个函数里面，对于 sysctl_tcp_low_latency 的判断，是不是要低时延地处理网络包

4. 如果 sysctl_tcp_low_latency 设置为 0，就放在 prequeue 队列中暂存，这样不用等待网络包处理完毕，就可以离开软中断的处理过程，但是会造成比较长的时延

5. 如果 sysctl_tcp_low_latency 设置为 1，则调用 tcp_v4_do_rcv。

6. 如果连接已经建立处于 TCP_ESTABLISHED 状态调用 tcp_rcv_established，其中调用 tcp_data_queue，将其放入 sk_receive_queue 队列进行处理，tcp_data_queue有四种情况

   * seq == tp->rcv_nxt：来的网络包正是服务端期望的下一个网络包。这个时候用户进程正在等待读取，就直接将网络包拷贝给用户进程。如果用户进程没有正在等待读取，或者因为内存原因没有能够拷贝成功，还是将网络包放入 sk_receive_queue 队列。对于乱序的数据只能暂时放在 out_of_order_queue 乱序队列中
   * end_seq 不大于 rcv_nxt：服务端期望网络包 5。但来了一个网络包 3，肯定是服务端早就收到了网络包 3，但是 ACK 没有到达客户端，中途丢了，客户端就认为网络包 3 没有发送成功，于是又发送了一遍，这种情况下，要赶紧给客户端再发送一次 ACK，表示早就收到了
   * seq 不小于 rcv_nxt + tcp_receive_window：客户端发送得太猛了。本来 seq应该在接收窗口里面，这样服务端才来得及处理，现在超出了接收窗口，说明客户端一下子把服务端给塞满了。服务端不能再接收数据包而只能发送 ACK ，在 ACK 中会将接收窗口为 0 的情况告知客户端，客户端就知道不能再发送。此时双方只能交互窗口探测数据包，直到服务端因为用户进程把数据读走了，空出接收窗口，才能在 ACK 里面再次告诉客户端，又有窗口又能发送数据包了
   * seq 小于 rcv_nxt && end_seq 大于 rcv_nxt：从 seq 到 rcv_nxt 这部分网络包原来的 ACK 客户端没有收到，所以重新发送了一次，从 rcv_nxt 到 end_seq 时新发送的，可以放入 sk_receive_queue 队列

   四种情况都排除掉了，说明网络包一定是一个乱序包

7. 如果是其他状态则调用 tcp_rcv_state_process

![img](https://static001.geekbang.org/resource/image/38/c6/385ff4a348dfd2f64feb0d7ba81e2bc6.png)

##### Socket 层

当接收的网络包进入各种队列之后，接下来就要等待用户进程去读取它们。读取一个 socket，就像读取一个文件一样，通过 read 系统调用。最终它会调用到用来表示一个打开文件的结构 stuct file 指向的 file_operations 操作。对于 socket 来讲，它的 file_operations 定义如下：

```
static const struct file_operations socket_file_ops = {
  .owner =  THIS_MODULE,
  .llseek =  no_llseek,
  .read_iter =  sock_read_iter,
  .write_iter =  sock_write_iter,
  .poll =    sock_poll,
  .unlocked_ioctl = sock_ioctl,
  .mmap =    sock_mmap,
  .release =  sock_close,
  .fasync =  sock_fasync,
  .sendpage =  sock_sendpage,
  .splice_write = generic_splice_sendpage,
  .splice_read =  sock_splice_read,
};
```

1. 在 sock_read_iter 中，通过 VFS 中的 struct file，将创建好的 socket 结构拿出来，然后调用 sock_recvmsg->sock_recvmsg_nosec
2. 调用socket 的 ops 的 recvmsg，根据 inet_stream_ops 的定义，调用 inet_recvmsg
3. 从 socket 结构，可以得到更底层的 sock 结构，然后调用 sk_prot 的 recvmsg 方法。根据 tcp_prot 的定义，调用 tcp_recvmsg
4. tcp_recvmsg 里面有一个 while 循环，不断地读取网络包。有三个队列，receive_queue 队列、prequeue 队列和 backlog 队列。需要把前一个队列处理完毕，才处理后一个队列
5. 先处理 sk_receive_queue 队列。如果找到了网络包就跳到 found_ok_skb。调用 skb_copy_datagram_msg，将网络包拷贝到用户进程中，然后直接进入下一层循环。直到 sk_receive_queue 队列处理完毕
6. 到了 sysctl_tcp_low_latency 判断。如果不需要低时延，则会有 prequeue 队列。跳到 do_prequeue 这里，调用 tcp_prequeue_process 进行处理。如果 sysctl_tcp_low_latency 设置为 1，即没有 prequeue 队列，或者 prequeue 队列为空，则需要处理 backlog 队列，在 release_sock 函数中会依次处理队列中的网络包