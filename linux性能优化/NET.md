# NET

### 关于 Linux 网络，你必须知道这些

##### 网络模型

开放式系统互联通信参考模型（Open System Interconnection Reference Model），简称为 OSI 网络模型：

* 应用层，负责为应用程序提供统一的接口
* 表示层，负责把数据转换成兼容接收系统的格式
* 会话层，负责维护计算机之间的通信连接
* 传输层，负责为数据加上传输表头，形成数据包
* 网络层，负责数据的路由和转发
* 数据链路层，负责 MAC 寻址、错误侦测和改错
* 物理层，负责在物理网络中传输数据帧

OSI 模型太复杂，也没提供一个可实现的方法。在 Linux 中，使用的是另一个更实用的四层模型，即 TCP/IP 网络模型：

* 应用层，负责向用户提供一组应用程序，比如 HTTP、FTP、DNS 等
* 传输层，负责端到端的通信，比如 TCP、UDP 等
* 网络层，负责网络包的封装、寻址和路由，比如 IP、ICMP 等
* 网络接口层，负责网络包在物理网络中的传输，比如 MAC 寻址、错误侦测以及通过网卡传输网络帧等

![img](https://static001.geekbang.org/resource/image/f2/bd/f2dbfb5500c2aa7c47de6216ee7098bd.png)

习惯上还是用 OSI 七层模型来描述。说到七层和四层负载均衡，对应的分别是 OSI 模型中的应用层和传输层

##### Linux 网络栈

有了 TCP/IP 模型后，在进行网络传输时，数据包就会按照协议栈，对上一层发来的数据进行逐层处理；然后封装上该层的协议头，再发送给下一层

![img](https://static001.geekbang.org/resource/image/c8/79/c8dfe80acc44ba1aa9df327c54349e79.png)

物理链路中并不能传输任意大小的数据包。网络接口配置的最大传输单元（MTU），就规定了最大的 IP 包大小。在以太网中，MTU 默认值是 1500（这也是 Linux 的默认值）。一旦网络包超过 MTU 的大小，就会在网络层分片，以保证分片后的 IP 包不大于 MTU 值。显然，MTU 越大，需要的分包也就越少，自然，网络吞吐能力就越好。

Linux 内核中的网络栈，其实也类似于 TCP/IP 的四层结构

![img](https://static001.geekbang.org/resource/image/c7/ac/c7b5b16539f90caabb537362ee7c27ac.png)

网卡是发送和接收网络包的基本设备。在系统启动过程中，网卡通过内核中的网卡驱动程序注册到系统中。而在网络收发过程中，内核通过中断跟网卡进行交互

##### Linux 网络收发流程

![img](https://static001.geekbang.org/resource/image/3a/65/3af644b6d463869ece19786a4634f765.png)

网络包的接收流程：

* 当一个网络帧到达网卡后，网卡会通过 DMA 方式，把这个网络包放到收包队列中；然后通过硬中断，告诉中断处理程序已经收到了网络包
* 网卡中断处理程序会为网络帧分配内核数据结构（sk_buff），并将其拷贝到 sk_buff 缓冲区中；然后再通过软中断，通知内核收到了新的网络帧
* 内核协议栈从缓冲区中取出网络帧，并通过网络协议栈，从下到上逐层处理这个网络帧
* 在链路层检查报文的合法性，找出上层协议的类型（比如 IPv4 还是 IPv6），再去掉帧头、帧尾，然后交给网络层
* 网络层取出 IP 头，判断网络包下一步的走向，比如是交给上层处理还是转发。当网络层确认这个包是要发送到本机后，就会取出上层协议的类型（比如 TCP 还是 UDP），去掉 IP 头，再交给传输层处理
* 传输层取出 TCP 头或者 UDP 头后，根据 < 源 IP、源端口、目的 IP、目的端口 > 四元组作为标识，找出对应的 Socket，并把数据拷贝到 Socket 的接收缓存中
* 最后，应用程序就可以使用 Socket 接口，读取到新接收到的数据了

网络包的发送流程：

* 应用程序调用 Socket API（比如 sendmsg）发送网络包。这是一个系统调用，所以会陷入到内核态的套接字层中。套接字层会把数据包放到 Socket 发送缓冲区中
* 网络协议栈从 Socket 发送缓冲区中，取出数据包；再按照 TCP/IP 栈，从上到下逐层处理
* 传输层和网络层，分别为其增加 TCP 头和 IP 头，执行路由查找确认下一跳的 IP，并按照 MTU 大小进行分片
* 分片后的网络包，再送到网络接口层，进行物理地址寻址，以找到下一跳的 MAC 地址。然后添加帧头和帧尾，放到发包队列中。这一切完成后，会有软中断通知驱动程序：发包队列中有新的网络帧需要发送
* 最后，驱动程序通过 DMA ，从发包队列中读出网络帧，并通过物理网卡把它发送出去

##### 性能指标

* 带宽，表示链路的最大传输速率，单位通常为 b/s （比特 / 秒）
* 吞吐量，表示单位时间内成功传输的数据量，单位通常为 b/s（比特 / 秒）或者 B/s（字节 / 秒）。吞吐量受带宽限制，而吞吐量 / 带宽，也就是该网络的使用率
* 延时，表示从网络请求发出后，一直到收到远端响应，所需要的时间延迟。在不同场景中，这一指标可能会有不同含义。比如，它可以表示，建立连接需要的时间（比如 TCP 握手延时），或一个数据包往返所需的时间（比如 RTT）
* PPS，是 Packet Per Second（包 / 秒）的缩写，表示以网络包为单位的传输速率。PPS 通常用来评估网络的转发能力，比如硬件交换机，通常可以达到线性转发（即 PPS 可以达到或者接近理论最大值）。而基于 Linux 服务器的转发，则容易受网络包大小的影响

除了这些指标，网络的可用性（网络能否正常通信）、并发连接数（TCP 连接数量）、丢包率（丢包百分比）、重传率（重新传输的网络包比例）等也是常用的性能指标

##### 网络配置

以网络接口 eth0 为例，ifconfig 和 ip 命令输出的指标基本相同，只是显示格式略微不同，更推荐使用 ip 工具，因为它提供了更丰富的功能和更易用的接口

```
$ ifconfig eth0
eth0: flags=4163<UP,BROADCAST,RUNNING,MULTICAST> mtu 1500
      inet 10.240.0.30 netmask 255.240.0.0 broadcast 10.255.255.255
      inet6 fe80::20d:3aff:fe07:cf2a prefixlen 64 scopeid 0x20<link>
      ether 78:0d:3a:07:cf:3a txqueuelen 1000 (Ethernet)
      RX packets 40809142 bytes 9542369803 (9.5 GB)
      RX errors 0 dropped 0 overruns 0 frame 0
      TX packets 32637401 bytes 4815573306 (4.8 GB)
      TX errors 0 dropped 0 overruns 0 carrier 0 collisions 0
​
$ ip -s addr show dev eth0
2: eth0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc mq state UP group default qlen 1000
  link/ether 78:0d:3a:07:cf:3a brd ff:ff:ff:ff:ff:ff
  inet 10.240.0.30/12 brd 10.255.255.255 scope global eth0
      valid_lft forever preferred_lft forever
  inet6 fe80::20d:3aff:fe07:cf2a/64 scope link
      valid_lft forever preferred_lft forever
  RX: bytes packets errors dropped overrun mcast
   9542432350 40809397 0       0       0       193
  TX: bytes packets errors dropped carrier collsns
   4815625265 32637658 0       0       0       0
```

* 第一，网络接口的状态标志。ifconfig 输出中的 RUNNING ，或 ip 输出中的 LOWER_UP ，都表示物理网络是连通的，即网卡已经连接到了交换机或者路由器中。如果看不到它们，通常表示网线被拔掉了
* 第二，MTU 的大小。MTU 默认大小是 1500，根据网络架构的不同（比如是否使用了 VXLAN 等叠加网络），可能需要调大或者调小 MTU 的数值
* 第三，网络接口的 IP 地址、子网以及 MAC 地址。都是保障网络功能正常工作所必需的，需要确保配置正确
* 第四，网络收发的字节数、包数、错误数以及丢包情况，特别是 TX 和 RX 部分的 errors、dropped、overruns、carrier 以及 collisions 等指标不为 0 时，通常表示出现了网络 I/O 问题。其中：
  * errors 表示发生错误的数据包数，比如校验错误、帧同步错误等
  * dropped 表示丢弃的数据包数，即数据包已经收到了 Ring Buffer，但因为内存不足等原因丢包
  * overruns 表示超限数据包数，即网络 I/O 速度过快，导致 Ring Buffer 中的数据包来不及处理（队列满）而导致的丢包
  * carrier 表示发生 carrirer 错误的数据包数，比如双工模式不匹配、物理电缆出现问题等
  * collisions 表示碰撞数据包数

##### 套接字信息

ifconfig 和 ip 只显示了网络接口收发数据包的统计信息，但在实际的性能问题中，网络协议栈中的统计信息，也必须关注。可以用 netstat 或者 ss ，来查看套接字、网络栈、网络接口以及路由表的信息，更推荐使用 ss 来查询网络的连接信息，因为它比 netstat 提供了更好的性能

```

# head -n 3 表示只显示前面3行
# -l 表示只显示监听套接字
# -n 表示显示数字地址和端口(而不是名字)
# -p 表示显示进程信息
$ netstat -nlp | head -n 3
Active Internet connections (only servers)
Proto Recv-Q Send-Q Local Address           Foreign Address         State       PID/Program name
tcp        0      0 127.0.0.53:53           0.0.0.0:*               LISTEN      840/systemd-resolve

# -l 表示只显示监听套接字
# -t 表示只显示 TCP 套接字
# -n 表示显示数字地址和端口(而不是名字)
# -p 表示显示进程信息
$ ss -ltnp | head -n 3
State    Recv-Q    Send-Q        Local Address:Port        Peer Address:Port
LISTEN   0         128           127.0.0.53%lo:53               0.0.0.0:*        users:(("systemd-resolve",pid=840,fd=13))
LISTEN   0         128                 0.0.0.0:22               0.0.0.0:*        users:(("sshd",pid=1459,fd=3))
```

其中，接收队列（Recv-Q）和发送队列（Send-Q）通常应该是 0。当发现它们不是 0 时，说明有网络包的堆积发生。在不同套接字状态下，它们的含义不同

当套接字处于连接状态（Established）时：

* Recv-Q 表示套接字缓冲还没有被应用程序取走的字节数（即接收队列长度）
* Send-Q 表示还没有被远端主机确认的字节数（即发送队列长度）

当套接字处于监听状态（Listening）时：

* Recv-Q 表示全连接队列的长度
* Send-Q 表示全连接队列的最大长度

全连接：是指服务器收到了客户端的 ACK，完成了 TCP 三次握手，然后就会把这个连接挪到全连接队列中。这些全连接中的套接字，还需要被 accept() 系统调用取走，服务器才可以开始真正处理客户端的请求

半连接：是指还没有完成 TCP 三次握手的连接，连接只进行了一半。服务器收到了客户端的 SYN 包后，就会把这个连接放到半连接队列中，然后再向客户端发送 SYN+ACK 包

##### 协议栈统计信息

使用 netstat 或 ss ，可以查看协议栈的信息

```
$ netstat -s
...
Tcp:
    3244906 active connection openings
    23143 passive connection openings
    115732 failed connection attempts
    2964 connection resets received
    1 connections established
    13025010 segments received
    17606946 segments sent out
    44438 segments retransmitted
    42 bad segments received
    5315 resets sent
    InCsumErrors: 42
...

$ ss -s
Total: 186 (kernel 1446)
TCP:   4 (estab 1, closed 0, orphaned 0, synrecv 0, timewait 0/0), ports 0

Transport Total     IP        IPv6
*    1446      -         -
RAW    2         1         1
UDP    2         2         0
TCP    4         3         1
...
```

##### 网络吞吐和 PPS

给 sar 增加 -n 参数就可以查看网络的统计信息，比如网络接口（DEV）、网络接口错误（EDEV）、TCP、UDP、ICMP 等等

```
// 数字1表示每隔1秒输出一组数据
$ sar -n DEV 1
Linux 4.15.0-1035-azure (ubuntu)   01/06/19   _x86_64_  (2 CPU)

13:21:40        IFACE   rxpck/s   txpck/s    rxkB/s    txkB/s   rxcmp/s   txcmp/s  rxmcst/s   %ifutil
13:21:41         eth0     18.00     20.00      5.79      4.25      0.00      0.00      0.00      0.00
13:21:41      docker0      0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00
13:21:41           lo      0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00
```

* IFACE表示网络接口的名称
* rxpck/s 和 txpck/s 分别是接收和发送的 PPS，单位为包 / 秒
* rxkB/s 和 txkB/s 分别是接收和发送的吞吐量，单位是 KB/ 秒
* rxcmp/s 和 txcmp/s 分别是接收和发送的压缩数据包数，单位是包 / 秒
* %ifutil 是网络接口的使用率，即半双工模式下为 (rxkB/s+txkB/s)/Bandwidth，而全双工模式下为 max(rxkB/s, txkB/s)/Bandwidth

Bandwidth 可以用 ethtool 来查询，它的单位通常是 Gb/s 或者 Mb/s，不过注意这里小写字母 b ，表示比特而不是字节。通常提到的千兆网卡、万兆网卡等，单位也都是比特。如下的 eth0 网卡就是一个千兆网卡

```
$ ethtool eth0 | grep Speed
  Speed: 1000Mb/s
```

##### 连通性和延时

通常使用 ping ，来测试远程主机的连通性和延时，这基于 ICMP 协议

```
// -c3表示发送三次ICMP包后停止
$ ping -c3 114.114.114.114
PING 114.114.114.114 (114.114.114.114) 56(84) bytes of data.
64 bytes from 114.114.114.114: icmp_seq=1 ttl=54 time=244 ms
64 bytes from 114.114.114.114: icmp_seq=2 ttl=47 time=244 ms
64 bytes from 114.114.114.114: icmp_seq=3 ttl=67 time=244 ms

--- 114.114.114.114 ping statistics ---
3 packets transmitted, 3 received, 0% packet loss, time 2001ms
rtt min/avg/max/mdev = 244.023/244.070/244.105/0.034 ms
```

* 第一部分，是每个 ICMP 请求的信息，包括 ICMP 序列号（icmp_seq）、TTL（生存时间，或者跳数）以及往返延时
* 第二部分，则是三次 ICMP 请求的汇总：发送了 3 个网络包，接收到 3 个响应，没有丢包发生，这说明测试主机到 114.114.114.114 是连通的；平均往返延时（RTT）是 244ms，也就是从发送 ICMP 开始，到接收到 114.114.114.114 回复的确认，总共经历 244ms

### C10K 和 C1000K 

C10K 就是单机同时处理 1 万个请求（并发连接 1 万）的问题，而 C1000K 也就是单机支持处理 100 万个请求（并发连接 100 万）的问题

##### I/O 模型优化

* 水平触发：只要文件描述符可以非阻塞地执行 I/O ，就会触发通知。应用程序可以随时检查文件描述符的状态，然后再根据状态，进行 I/O 操作。select 和 poll 需要从文件描述符列表中，找出哪些可以执行 I/O ，然后进行真正的网络 I/O 读写。由于 I/O 是非阻塞的，一个线程中就可以同时监控一批套接字的文件描述符，这样就达到了单线程处理多请求的目的
  * 优点：是对应用程序比较友好，它的 API 非常简单
  * 缺陷：需要对这些文件描述符列表进行轮询，请求数多的时候就会比较耗时。并且，select 和 poll 还有一些其他的限制，如文件描述符个数上限限制
* 边缘触发：只有在文件描述符的状态发生改变（也就是 I/O 请求达到）时，才发送一次通知。这时候，应用程序需要尽可能多地执行 I/O，直到无法继续读写，才可以停止。如果 I/O 没执行完，或者因为某种原因没来得及处理，那么这次通知也就丢失了。 epoll 很好地解决了select/poll的问题。epoll 使用红黑树，在内核中管理文件描述符的集合，就不需要应用程序在每次操作时都传入、传出这个集合。epoll 使用事件驱动的机制，只关注有 I/O 事件发生的文件描述符，不需要轮询扫描整个集合
* 异步 I/O（Asynchronous I/O，简称为 AIO）：要使用的话，一定要小心设计，使用难度比较高

##### 工作模型优化

* 主进程 + 多个 worker 子进程：最常用的一种模型。一个通用工作模式就是：主进程执行 bind() + listen() 后，创建多个子进程；在每个子进程中，都通过 accept() 或 epoll_wait() ，来处理相同的套接字。最常用的反向代理服务器 Nginx 就是这么工作的。accept() 和 epoll_wait() 调用，存在一个惊群的问题。当网络 I/O 事件发生时，多个进程被同时唤醒，但实际上只有一个进程来响应这个事件，其他被唤醒的进程都会重新休眠。其中，accept() 的惊群问题，已经在 Linux 2.6 中解决了；而 epoll 的问题，到了 Linux 4.5 ，才通过 EPOLLEXCLUSIVE 解决

![img](https://static001.geekbang.org/resource/image/45/7e/451a24fb8f096729ed6822b1615b097e.png)

* 监听到相同端口的多进程模型：在这种方式下，所有的进程都监听相同的接口，并且开启 SO_REUSEPORT 选项，由内核负责将请求负载均衡到这些监听进程中去，内核确保了只有一个进程被唤醒，就不会出现惊群问题

![img](https://static001.geekbang.org/resource/image/90/bd/90df0945f6ce5c910ae361bf2b135bbd.png)

##### C1000K

基于 I/O 多路复用和请求处理的优化，C10K 问题很容易就可以解决

* 首先从物理资源使用上来说，100 万个请求需要大量的系统资源。假设每个请求需要 16KB 内存，总共需要大约 15 GB 内存。而从带宽上来说，假设只有 20% 活跃连接，即使每个连接只需要 1KB/s 的吞吐量，总共也需要 1.6 Gb/s 的吞吐量。千兆网卡显然满足不了这么大的吞吐量，所以还需要配置万兆网卡，或者基于多网卡 Bonding 承载更大的吞吐量
* 从软件资源上来说，大量的连接也会占用大量的软件资源，比如文件描述符的数量、连接状态的跟踪（CONNTRACK）、网络协议栈的缓存大小（比如套接字读写缓存、TCP 读写缓存）等等
* 最后，大量请求带来的中断处理，也会带来非常高的处理成本

C1000K 的解决方法，本质上还是构建在 epoll 的非阻塞 I/O 模型上。只不过，除了 I/O 模型之外，还需要从应用程序到 Linux 内核、再到 CPU、内存和网络等各个层次的深度优化

##### C10M

在 C1000K 问题中，各种软件、硬件的优化很可能都已经做到头了。特别是当升级完硬件（比如足够多的内存、带宽足够大的网卡、更多的网络功能卸载等）后，无论怎么优化应用程序和内核中的各种网络参数，想实现 1000 万请求的并发，都是极其困难的。要解决这个问题，最重要就是跳过内核协议栈的冗长路径，把网络包直接送到要处理的应用程序那里去。这里有两种常见的机制

* DPDK：是用户态网络的标准。它跳过内核协议栈，直接由用户态进程通过轮询的方式，来处理网络接收，说起轮询，它的低效主要体现在哪里呢？是查询时间明显多于实际工作时间！换个角度来想，如果每时每刻都有新的网络包需要处理，轮询的优势就很明显了
* XDP： Linux 内核提供的一种高性能网络数据路径。它允许网络包，在进入内核协议栈之前，就进行处理，也可以带来更高的性能。XDP 底层跟 bcc-tools 一样，都是基于 Linux 内核的 eBPF 机制实现的

### 怎么评估系统的网络性能？

##### 网络基准测试

* 基于 HTTP 或者 HTTPS 的 Web 应用程序，显然属于应用层，需要测试 HTTP/HTTPS 的性能
* 大多数游戏服务器来说，为了支持更大的同时在线人数，通常会基于 TCP 或 UDP ，与客户端进行交互，需要测试 TCP/UDP 的性能
* 把 Linux 作为一个软交换机或者路由器用。更关注网络包的处理能力（即 PPS），重点关注网络层的转发性能

低层协议是其上的各层网络协议的基础。自然，低层协议的性能，也就决定了高层的网络性能

##### 各协议层的性能测试

1. 转发性能

网络接口层和网络层，它们主要负责网络包的封装、寻址、路由以及发送和接收。在这两个网络协议层中，每秒可处理的网络包数 PPS，就是最重要的性能指标。特别是 64B 小包的处理能力，值得特别关注

分析工具：

* hping3 ：不仅可以作为一个 SYN 攻击的工具来使用。还是一个测试网络包处理能力的性能工具
* pktgen：Linux 内核自带的高性能网络测试工具 。支持丰富的自定义选项，方便根据实际需要构造所需网络包，从而更准确地测试出目标服务器的性能

不能直接找到 pktgen 命令。因为 pktgen 作为一个内核线程来运行，需要你加载 pktgen 内核模块后，再通过 /proc 文件系统来交互

```
$ modprobe pktgen
$ ps -ef | grep pktgen | grep -v grep
root     26384     2  0 06:17 ?        00:00:00 [kpktgend_0]
root     26385     2  0 06:17 ?        00:00:00 [kpktgend_1]
$ ls /proc/net/pktgen/
kpktgend_0  kpktgend_1  pgctrl
// 说明有两个CPU
// 如果 modprobe 命令执行失败，说明内核没有配置 CONFIG_NET_PKTGEN 选项
// pktgen 在每个 CPU 上启动一个内核线程，并可以通过 /proc/net/pktgen 下面的同名文件，跟这些线程交互；而 pgctrl 则主要用来控制这次测试的开启和停止
```

```
// 如下所有拷贝到shell中运行，或者放置到一个shell脚本中去
# 定义一个工具函数，方便后面配置各种测试选项
function pgset() {
    local result
    echo $1 > $PGDEV

    result=`cat $PGDEV | fgrep "Result: OK:"`
    if [ "$result" = "" ]; then
         cat $PGDEV | fgrep Result:
    fi
}

# 为0号线程绑定eth0网卡
PGDEV=/proc/net/pktgen/kpktgend_0
pgset "rem_device_all"   # 清空网卡绑定
pgset "add_device eth0"  # 添加eth0网卡

# 配置eth0网卡的测试选项
PGDEV=/proc/net/pktgen/eth0
pgset "count 1000000"    # 总发包数量
pgset "delay 5000"       # 不同包之间的发送延迟(单位纳秒)
pgset "clone_skb 0"      # SKB包复制
pgset "pkt_size 64"      # 网络包大小
pgset "dst 192.168.0.30" # 目的IP
pgset "dst_mac 11:11:11:11:11:11"  # 目的MAC

# 启动测试
PGDEV=/proc/net/pktgen/pgctrl
pgset "start"
```

测试完成后，结果可以从 /proc 文件系统中获取

```
$ cat /proc/net/pktgen/eth0
Params: count 1000000  min_pkt_size: 64  max_pkt_size: 64
     frags: 0  delay: 0  clone_skb: 0  ifname: eth0
     flows: 0 flowlen: 0
...
Current:
     pkts-sofar: 1000000  errors: 0
     started: 1534853256071us  stopped: 1534861576098us idle: 70673us
...
Result: OK: 8320027(c8249354+d70673) usec, 1000000 (64byte,0frags)
  120191pps 61Mb/sec (61537792bps) errors: 0
  
第一部分的 Params 是测试选项
第二部分的 Current 是测试进度，packts so far（pkts-sofar）表示已经发送了 100 万个包，表明测试已完成
第三部分的 Result 是测试结果，包含测试所用时间、网络包数量和分片、PPS、吞吐量以及错误数  

千兆交换机的 PPS。交换机可以达到线速（满负载时，无差错转发），它的 PPS 就是 1000Mbit 除以以太网帧的大小，即 1000Mbps/((64+20)*8bit) = 1.5 Mpps（其中，20B 为以太网帧前导和帧间距的大小）。千兆交换机的 PPS，可以达到 150 万 PPS，比12 万大多了。现在的多核服务器和万兆网卡已经很普遍了，稍做优化就可以达到数百万的 PPS。而且，如果用 DPDK 或 XDP ，还能达到千万数量级
```

2. TCP/UDP 性能

分析工具：

* iperf 和 netperf 都是最常用的网络性能测试工具，测试 TCP 和 UDP 的吞吐量。它们都以客户端和服务器通信的方式，测试一段时间内的平均吞吐量

```
# Ubuntu
apt-get install iperf3


# 服务器端：-s表示启动服务端，-i表示汇报间隔，-p表示监听端口
$ iperf3 -s -i 1 -p 10000

# 客户端：-c表示启动客户端，192.168.0.30为目标服务器的IP
# -b表示目标带宽(单位是bits/s)
# -t表示测试时间
# -P表示并发数，-p表示目标服务器监听端口
$ iperf3 -c 192.168.0.30 -b 1G -t 15 -P 2 -p 10000

# 查看 iperf 的报告
[ ID] Interval           Transfer     Bandwidth
...
[SUM]   0.00-15.04  sec  0.00 Bytes  0.00 bits/sec                  sender
[SUM]   0.00-15.04  sec  1.51 GBytes   860 Mbits/sec                  receiver

汇总结果，包括测试时间、数据传输量以及带宽等。按照发送和接收，又分为了 sender 和 receiver 两行。从测试结果可以看到，这台机器 TCP 接收的带宽（吞吐量）为 860 Mb/s， 跟目标的 1Gb/s 相比，还是有些差距
```

3. HTTP 性能

分析工具：

* ab、webbench：都是常用的 HTTP 压力测试工具。ab 是 Apache 自带的 HTTP 压测工具，主要测试 HTTP 服务的每秒请求数、请求延迟、吞吐量以及请求延迟的分布情况

```
# Ubuntu
$ apt-get install -y apache2-utils

# 使用 Docker 启动一个 Nginx 服务，然后用 ab 来测试它的性能
$ docker run -p 80:80 -itd nginx

# 另一台机器上，运行 ab 命令，测试 Nginx 的性能
# -c表示并发请求数为1000，-n表示总的请求数为10000
$ ab -c 1000 -n 10000 http://192.168.0.30/
...
Server Software:        nginx/1.15.8
Server Hostname:        192.168.0.30
Server Port:            80

...

Requests per second:    1078.54 [#/sec] (mean)
Time per request:       927.183 [ms] (mean)
Time per request:       0.927 [ms] (mean, across all concurrent requests)
Transfer rate:          890.00 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0   27 152.1      1    1038
Processing:     9  207 843.0     22    9242
Waiting:        8  207 843.0     22    9242
Total:         15  233 857.7     23    9268

Percentage of the requests served within a certain time (ms)
  50%     23
  66%     24
  75%     24
  80%     26
  90%    274
  95%   1195
  98%   2335
  99%   4663
 100%   9268 (longest request)
 
ab 的测试结果分为三个部分，分别是请求汇总、连接时间汇总还有请求延迟汇总
请求汇总部分：Requests per second 为 1074；每个请求的延迟（Time per request）分为两行，第一行的 927 ms 表示平均延迟，包括了线程运行的调度时间和网络请求响应时间，而下一行的 0.927ms ，则表示实际请求的响应时间；Transfer rate 表示吞吐量（BPS）为 890 KB/s
连接时间汇总部分：展示了建立连接、请求、等待以及汇总等的各类时间，包括最小、最大、平均以及中值处理时间
请求延迟汇总部分：给出了不同时间段内处理请求的百分比，比如， 90% 的请求，都可以在 274ms 内完成
```

4. 应用负载性能

用 iperf 或者 ab 等测试工具，得到 TCP、HTTP 等的性能数据后，这些无法代表应用程序的实际性能。为了得到应用程序的实际性能，就要求性能工具本身可以模拟用户的请求负载，而 iperf、ab 这类工具就无能为力了。幸运的是，可以用 wrk、TCPCopy、Jmeter 或者 LoadRunner 等实现这个目标

分析工具：

* wrk：一个 HTTP 性能测试工具，内置了 LuaJIT，方便根据实际需求，生成所需的请求负载，或者自定义响应的处理方法，wrk 工具本身不提供 yum 或 apt 的安装方法，需要通过源码编译来安装

```
$ git clone https://github.com/wg/wrk
$ cd wrk
$ apt-get install build-essential -y
$ make
$ sudo cp wrk /usr/local/bin/
```

```
# 用 wrk ，来重新测一下前面已经启动的 Nginx 的性能
# -c表示并发连接数1000，-t表示线程数为2
$ wrk -c 1000 -t 2 http://192.168.0.30/
Running 10s test @ http://192.168.0.30/
  2 threads and 1000 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency    65.83ms  174.06ms   1.99s    95.85%
    Req/Sec     4.87k   628.73     6.78k    69.00%
  96954 requests in 10.06s, 78.59MB read
  Socket errors: connect 0, read 0, write 0, timeout 179
Requests/sec:   9641.31
Transfer/sec:      7.82MB

使用 2 个线程、并发 1000 连接，重新测试了 Nginx 的性能。每秒请求数为 9641，吞吐量为 7.82MB，平均延迟为 65ms，比前面 ab 的测试结果要好很多。说明性能工具本身的性能，对性能测试也是至关重要的。不合适的性能工具，并不能准确测出应用程序的最佳性能
```

### DNS 解析时快时慢，该怎么办？

DNS（Domain Name System），即域名系统，是互联网中最基础的一项服务，主要提供域名和 IP 地址之间映射关系的查询服务。DNS 不仅方便了人们访问不同的互联网服务，更为很多应用提供了，动态服务发现和全局负载均衡（Global Server Load Balance，GSLB）的机制。这样，DNS 就可以选择离用户最近的 IP 来提供服务。即使后端服务的 IP 地址发生变化，用户依然可以用相同域名来访问

##### 域名与 DNS 解析

DNS 协议在 TCP/IP 栈中属于应用层，不过实际传输还是基于 UDP 或者 TCP 协议（UDP 居多） ，并且域名服务器一般监听在端口 53 上。域名解析是用递归的方式（从顶级开始，以此类推），发送给每个层级的域名服务器，直到得到解析结果。递归查询的过程 DNS 服务器会完成，需要预先配置一个可用的 DNS 服务器。当然，通常来说，每级 DNS 服务器，都会有最近解析记录的缓存。当缓存命中时，直接用缓存中的记录应答就可以了。如果缓存过期或者不存在，才需要用刚刚提到的递归方式查询。系统管理员在配置 Linux 系统的网络时，除了需要配置 IP 地址，还需要给它配置 DNS 服务器，这样它才可以通过域名来访问外部服务

```
// 查询系统配置
$ cat /etc/resolv.conf
nameserver 114.114.114.114
```

DNS 服务通过资源记录的方式，来管理所有数据，它支持 A、CNAME、MX、NS、PTR 等多种类型的记录。比如：A 记录，用来把域名转换成 IP 地址；CNAME 记录，用来创建别名；而 NS 记录，则表示该域名对应的域名服务器地址

```

$ nslookup time.geekbang.org
# 域名服务器及端口信息
Server:    114.114.114.114
Address:  114.114.114.114#53

# 非权威查询结果
Non-authoritative answer:
Name:  time.geekbang.org
Address: 39.106.233.17
```

分析工具：

* nslookup：可以查询到这个域名的 A 记录
* dig：提供了 trace 功能，可以展示递归查询的整个过程

```
# +trace表示开启跟踪查询
# +nodnssec表示禁止DNS安全扩展
$ dig +trace +nodnssec time.geekbang.org

; <<>> DiG 9.11.3-1ubuntu1.3-Ubuntu <<>> +trace +nodnssec time.geekbang.org
;; global options: +cmd
.      322086  IN  NS  m.root-servers.net.
.      322086  IN  NS  a.root-servers.net.
.      322086  IN  NS  i.root-servers.net.
.      322086  IN  NS  d.root-servers.net.
.      322086  IN  NS  g.root-servers.net.
.      322086  IN  NS  l.root-servers.net.
.      322086  IN  NS  c.root-servers.net.
.      322086  IN  NS  b.root-servers.net.
.      322086  IN  NS  h.root-servers.net.
.      322086  IN  NS  e.root-servers.net.
.      322086  IN  NS  k.root-servers.net.
.      322086  IN  NS  j.root-servers.net.
.      322086  IN  NS  f.root-servers.net.
;; Received 239 bytes from 114.114.114.114#53(114.114.114.114) in 1340 ms

org.      172800  IN  NS  a0.org.afilias-nst.info.
org.      172800  IN  NS  a2.org.afilias-nst.info.
org.      172800  IN  NS  b0.org.afilias-nst.org.
org.      172800  IN  NS  b2.org.afilias-nst.org.
org.      172800  IN  NS  c0.org.afilias-nst.info.
org.      172800  IN  NS  d0.org.afilias-nst.org.
;; Received 448 bytes from 198.97.190.53#53(h.root-servers.net) in 708 ms

geekbang.org.    86400  IN  NS  dns9.hichina.com.
geekbang.org.    86400  IN  NS  dns10.hichina.com.
;; Received 96 bytes from 199.19.54.1#53(b0.org.afilias-nst.org) in 1833 ms

time.geekbang.org.  600  IN  A  39.106.233.176
;; Received 62 bytes from 140.205.41.16#53(dns10.hichina.com) in 4 ms

dig trace 的输出，主要包括四部分
第一部分，是从 114.114.114.114 查到的一些根域名服务器（.）的 NS 记录
第二部分，是从 NS 记录结果中选一个（h.root-servers.net），并查询顶级域名 org. 的 NS 记录
第三部分，是从 org. 的 NS 记录中选择一个（b0.org.afilias-nst.org），并查询二级域名 geekbang.org. 的 NS 服务器
最后一部分，是从 geekbang.org. 的 NS 服务器（dns10.hichina.com）查询最终主机 time.geekbang.org. 的 A 记录
```

![img](https://static001.geekbang.org/resource/image/5f/d3/5ffda41ec62fc3c9e0de3fa3443c9cd3.png)

不仅仅是发布到互联网的服务需要域名，很多时候，我们也希望能对局域网内部的主机进行域名解析（即内网域名，大多数情况下为主机名）。Linux 支持这种行为。所以，可以把主机名和 IP 地址的映射关系，写入本机的 /etc/hosts 文件中。这样，指定的主机名就可以在本地直接找到目标 IP。比如，可以执行下面的命令来操作

```
$ cat /etc/hosts
127.0.0.1   localhost localhost.localdomain
::1         localhost6 localhost6.localdomain6
192.168.0.100 domain.com
```

还可以在内网中，搭建自定义的 DNS 服务器，专门用来解析内网中的域名。而内网 DNS 服务器，一般还会设置一个或多个上游 DNS 服务器，用来解析外网的域名

##### 案例分析

1. DNS 解析失败

```
# nslookup time.geekbang.org
;; connection timed out; no servers could be reached

# ping -c3 114.114.114.114
PING 114.114.114.114 (114.114.114.114): 56 data bytes
64 bytes from 114.114.114.114: icmp_seq=0 ttl=56 time=31.116 ms
64 bytes from 114.114.114.114: icmp_seq=1 ttl=60 time=31.245 ms
64 bytes from 114.114.114.114: icmp_seq=2 ttl=68 time=31.128 ms
--- 114.114.114.114 ping statistics ---
3 packets transmitted, 3 packets received, 0% packet loss
round-trip min/avg/max/stddev = 31.116/31.163/31.245/0.058 ms

网络是通的。知道 nslookup 命令失败的原因:
# nslookup -debug time.geekbang.org
;; Connection to 127.0.0.1#53(127.0.0.1) for time.geekbang.org failed: connection refused.
;; Connection to ::1#53(::1) for time.geekbang.org failed: address not available.

nslookup 连接环回地址（127.0.0.1 和 ::1）的 53 端口失败。为什么会去连接环回地址，而不是先前看到的 114.114.114.114，可能没有配置 DNS 服务器

# cat /etc/resolv.conf
没有任何输出，果然没有配置 DNS 服务器
```

```
# 加一个 time 命令，输出解析所用时间
# time nslookup time.geekbang.org
Server:    8.8.8.8
Address:  8.8.8.8#53

Non-authoritative answer:
Name:  time.geekbang.org
Address: 39.106.233.176

real  0m10.349s
user  0m0.004s
sys  0m0.0

解析非常慢，居然用了 10 秒，多次运行上面的 nslookup 命令，可能偶尔还会碰到下面这种错误：NS 解析的结果不但比较慢，而且还会发生超时失败的情况
# time nslookup time.geekbang.org
;; connection timed out; no servers could be reached

real  0m15.011s
user  0m0.006s
sys  0m0.006s

可能的原因：
DNS 服务器本身有问题，响应慢并且不稳定
客户端到 DNS 服务器的网络延迟比较大
DNS 请求或者响应包，在某些情况下被链路中的网络设备弄丢了

客户端连接的 DNS 是 8.8.8.8，这是 Google 提供的 DNS 服务。出问题的概率应该比较小
ping 可以用来测试服务器的延迟
# ping -c3 8.8.8.8
PING 8.8.8.8 (8.8.8.8): 56 data bytes
64 bytes from 8.8.8.8: icmp_seq=0 ttl=31 time=137.637 ms
64 bytes from 8.8.8.8: icmp_seq=1 ttl=31 time=144.743 ms
64 bytes from 8.8.8.8: icmp_seq=2 ttl=31 time=138.576 ms
--- 8.8.8.8 ping statistics ---
3 packets transmitted, 3 packets received, 0% packet loss
round-trip min/avg/max/stddev = 137.637/140.319/144.743/3.152 ms

延迟已经达到了 140ms，多次运行上面的 ping 测试，还会看到偶尔出现的丢包现象，这进一步解释了，为什么 nslookup 偶尔会失败，正是网络链路中的丢包导致的，换一个延迟更小的 DNS 服务器，比如电信提供的 114.114.114.114

# ping -c3 114.114.114.114
PING 114.114.114.114 (114.114.114.114): 56 data bytes
64 bytes from 114.114.114.114: icmp_seq=0 ttl=67 time=31.130 ms
64 bytes from 114.114.114.114: icmp_seq=1 ttl=56 time=31.302 ms
64 bytes from 114.114.114.114: icmp_seq=2 ttl=56 time=31.250 ms
--- 114.114.114.114 ping statistics ---
3 packets transmitted, 3 packets received, 0% packet loss
round-trip min/avg/max/stddev = 31.130/31.227/31.302/0.072 ms

延迟的确小了很多。继续执行下面的命令，更换 DNS 服务器，然后，再次执行 nslookup 解析命令
# echo nameserver 114.114.114.114 > /etc/resolv.conf
# time nslookup time.geekbang.org
Server:    114.114.114.114
Address:  114.114.114.114#53

Non-authoritative answer:
Name:  time.geekbang.org
Address: 39.106.233.176

real    0m0.064s
user    0m0.007s
sys     0m0.006s

只需要 64ms 就可以完成解析，比刚才的 10s 要好很多，如果多次运行 nslookup 命令，估计就不是每次都有好结果了。有时候需要 1s 甚至更多的时间，1s 的 DNS 解析时间太长了，对很多应用来说也是不可接受的。需要使用 DNS 缓存

启动 dnsmasq，修改 /etc/resolv.conf，将 DNS 服务器改为 dnsmasq 的监听地址，这儿是 127.0.0.1。接着，重新执行多次 nslookup 命令
# echo nameserver 127.0.0.1 > /etc/resolv.conf
# time nslookup time.geekbang.org
Server:    127.0.0.1
Address:  127.0.0.1#53

Non-authoritative answer:
Name:  time.geekbang.org
Address: 39.106.233.176

real  0m0.492s
user  0m0.007s
sys  0m0.006s

# time nslookup time.geekbang.org
Server:    127.0.0.1
Address:  127.0.0.1#53

Non-authoritative answer:
Name:  time.geekbang.org
Address: 39.106.233.176

real  0m0.011s
user  0m0.008s
sys  0m0.003s
只有第一次的解析很慢，需要 0.5s，以后的每次解析都很快，只需要 11ms。后面每次 DNS 解析需要的时间也都很稳定
```

dnsmasq 是最常用的 DNS 缓存服务之一，还经常作为 DHCP 服务来使用。它的安装和配置都比较简单，性能也可以满足绝大多数应用程序对 DNS 缓存的需求，主流 Linux 发行版，除了最新版本的 Ubuntu （如 18.04 或者更新版本）外，其他版本并没有自动配置 DNS 缓存

