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

DNS 解析失败

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

### 怎么使用 tcpdump 和 Wireshark 分析网络流量？

分析工具：

* tcpdump：仅支持命令行格式使用，常用在服务器中抓取和分析网络包
* Wireshark：除了可以抓包外，还提供了强大的图形界面和汇总分析工具，在分析复杂的网络情景时，尤为简单和实用

##### 案例分析

```
# 禁止接收从DNS服务器发送过来并包含googleusercontent的包
$ iptables -I INPUT -p udp --sport 53 -m string --string googleusercontent --algo bm -j DROP

# ping 3 次（默认每次发送间隔1秒）
# 假设DNS服务器还是上一期配置的114.114.114.114
$ ping -c3 geektime.org
PING geektime.org (35.190.27.188) 56(84) bytes of data.
64 bytes from 35.190.27.188 (35.190.27.188): icmp_seq=1 ttl=43 time=36.8 ms
64 bytes from 35.190.27.188 (35.190.27.188): icmp_seq=2 ttl=43 time=31.1 ms
64 bytes from 35.190.27.188 (35.190.27.188): icmp_seq=3 ttl=43 time=31.2 ms

--- geektime.org ping statistics ---
3 packets transmitted, 3 received, 0% packet loss, time 11049ms
rtt min/avg/max/mdev = 31.146/33.074/36.809/2.649 ms

3 次发送，3 次响应，没有丢包，但三次发送和接受的总时间居然超过了 11s（11049ms）,可能是 DNS 解析缓慢的问题
$ time nslookup geektime.org
Server:    114.114.114.114
Address:  114.114.114.114#53

Non-authoritative answer:
Name:  geektime.org
Address: 35.190.27.188

real  0m0.044s
user  0m0.006s
sys  0m0.003s

域名解析还是很快的，只需要 44ms，显然比 11s 短了很多
# 另一个终端执行
$ tcpdump -nn udp port 53 or host 35.190.27.188

$ ping -c3 geektime.org
...
--- geektime.org ping statistics ---
3 packets transmitted, 3 received, 0% packet loss, time 11095ms
rtt min/avg/max/mdev = 81.473/81.572/81.757/0.130 ms

# 查看 tcpdump 的输出
tcpdump: verbose output suppressed, use -v or -vv for full protocol decode
listening on eth0, link-type EN10MB (Ethernet), capture size 262144 bytes
14:02:31.100564 IP 172.16.3.4.56669 > 114.114.114.114.53: 36909+ A? geektime.org. (30)
14:02:31.507699 IP 114.114.114.114.53 > 172.16.3.4.56669: 36909 1/0/0 A 35.190.27.188 (46)
14:02:31.508164 IP 172.16.3.4 > 35.190.27.188: ICMP echo request, id 4356, seq 1, length 64
14:02:31.539667 IP 35.190.27.188 > 172.16.3.4: ICMP echo reply, id 4356, seq 1, length 64
14:02:31.539995 IP 172.16.3.4.60254 > 114.114.114.114.53: 49932+ PTR? 188.27.190.35.in-addr.arpa. (44)
14:02:36.545104 IP 172.16.3.4.60254 > 114.114.114.114.53: 49932+ PTR? 188.27.190.35.in-addr.arpa. (44)
14:02:41.551284 IP 172.16.3.4 > 35.190.27.188: ICMP echo request, id 4356, seq 2, length 64
14:02:41.582363 IP 35.190.27.188 > 172.16.3.4: ICMP echo reply, id 4356, seq 2, length 64
14:02:42.552506 IP 172.16.3.4 > 35.190.27.188: ICMP echo request, id 4356, seq 3, length 64
14:02:42.583646 IP 35.190.27.188 > 172.16.3.4: ICMP echo reply, id 4356, seq 3, length 64

第一行的记录：
36909+ 表示查询标识值，它也会出现在响应中，加号表示启用递归查询
A? 表示查询 A 记录
geektime.org. 表示待查询的域名
30 表示报文长度

第二行是从 114.114.114.114 发送回来的 DNS 响应——域名 geektime.org. 的 A 记录值为 35.190.27.188
第三行和第四行，是 ICMP echo request 和 ICMP echo reply，响应包的时间戳 14:02:31.539667，减去请求包的时间戳 14:02:31.508164 ，就可以得到，这次 ICMP 所用时间为 30ms。看起来并没有问题

随后的两条反向地址解析 PTR 请求，比较可疑。只看到了请求包，却没有应答包。这两条记录都是发出后 5s 才出现下一个网络包，两条 PTR 记录就消耗了 10s

最后的四个包，则是两次正常的 ICMP 请求和响应，根据时间戳计算其延迟，也是 30ms

根源，两次 PTR 请求没有得到响应而超时导致的。PTR 反向地址解析的目的，是从 IP 地址反查出域名，但事实上，并非所有 IP 地址都会定义 PTR 记录，所以 PTR 查询很可能会失败。所以，在使用 ping 时，如果发现结果中的延迟并不大，而 ping 命令本身却很慢，有可能是背后的 PTR 在搞鬼。只要禁止 PTR 就可以

$ ping -n -c3 geektime.org
PING geektime.org (35.190.27.188) 56(84) bytes of data.
64 bytes from 35.190.27.188: icmp_seq=1 ttl=43 time=33.5 ms
64 bytes from 35.190.27.188: icmp_seq=2 ttl=43 time=39.0 ms
64 bytes from 35.190.27.188: icmp_seq=3 ttl=43 time=32.8 ms

--- geektime.org ping statistics ---
3 packets transmitted, 3 received, 0% packet loss, time 2002ms
rtt min/avg/max/mdev = 32.879/35.160/39.030/2.755 ms

现在只需要 2s 就可以结束，比刚才的 11s 快多了

开始时，执行了 iptables 命令，那也不要忘了删掉它
$ iptables -D INPUT -p udp --sport 53 -m string --string googleusercontent --algo bm -j DROP

如果换一个 DNS 服务器，就可以用 PTR 反查到 35.190.27.188 所对应的域名
$ nslookup -type=PTR 35.190.27.188 8.8.8.8
Server:  8.8.8.8
Address:  8.8.8.8#53
Non-authoritative answer:
188.27.190.35.in-addr.arpa  name = 188.27.190.35.bc.googleusercontent.com.
Authoritative answers can be found from:

结果并非 geekbang.org，而是 188.27.190.35.bc.googleusercontent.com。这也是为什么，开始时将包含 googleusercontent 的丢弃后，ping 就慢了。因为 iptables ，实际上是把 PTR 响应给丢了，所以导致 PTR 请求超时
```

##### tcpdump

tcpdump 的输出格式

```
时间戳 协议 源地址.源端口 > 目的地址.目的端口 网络包详细信息
```

![img](https://static001.geekbang.org/resource/image/85/ff/859d3b5c0071335429620a3fcdde4fff.png)

![img](https://static001.geekbang.org/resource/image/48/b3/4870a28c032bdd2a26561604ae2f7cb3.png)

##### Wireshark

执行下面的命令，把抓取的网络包保存到 ping.pcap 文件中：

```
$ tcpdump -nn udp port 53 or host 35.190.27.188 -w ping.pcap
```

把它拷贝到你安装有 Wireshark 的机器中：

```
$ scp host-ip/path/ping.pcap .
```

再用 Wireshark 打开它

![img](https://static001.geekbang.org/resource/image/6b/2c/6b854703dcfcccf64c0a69adecf2f42c.png)

它不仅以更规整的格式，展示了各个网络包的头部信息；还用了不同颜色，展示 DNS 和 ICMP 这两种不同的协议。可以一眼看出，中间的两条 PTR 查询并没有响应包

在网络包列表中选择某一个网络包后，在其下方的网络包详情中，还可以看到，这个包在协议栈各层的详细信息

![img](https://static001.geekbang.org/resource/image/59/25/59781a5dc7b1b9234643991365bfc925.png)

##### 案例分析

```
// 终端一，执行下面的命令，首先查出 example.com 的 IP。然后，执行 tcpdump 命令，过滤得到的 IP 地址，并将结果保存到 web.pcap 中
$ dig +short example.com
93.184.216.34
$ tcpdump -nn host 93.184.216.34 -w web.pcap  // 或者 tcpdump -nn host example.com -w web.pcap

// 换到终端二，执行下面的 curl 命令
$ curl http://example.com

// 回到终端一，按下 Ctrl+C 停止 tcpdump，查看
```

![img](https://static001.geekbang.org/resource/image/07/9d/07bcdba5b563ebae36f5b5b453aacd9d.png)

点击 Statistics -> Flow Graph，然后，在弹出的界面中的 Flow type 选择 TCP Flows，你可以更清晰的看到，整个过程中 TCP 流的执行过程

![img](https://static001.geekbang.org/resource/image/4e/bb/4ec784752fdbc0cc5ead036a6419cbbb.png)

作为对比， 通常看到的 TCP 三次握手和四次挥手的流程，基本是这样的：

![img](https://static001.geekbang.org/resource/image/52/e8/5230fb678fcd3ca6b55d4644881811e8.png)

这里抓到的包跟上面的四次挥手，并不完全一样，实际挥手过程只有三个包，而不是四个。其实，是因为服务器端收到客户端的 FIN 后，服务器端同时也要关闭连接，这样就可以把 ACK 和 FIN 合并到一起发送，节省了一个包，变成了“三次挥手”。通常情况下，服务器端收到客户端的 FIN 后，很可能还没发送完数据，所以就会先回复客户端一个 ACK 包。稍等一会儿，完成所有数据包的发送后，才会发送 FIN 包。这也就是四次挥手了

![img](https://static001.geekbang.org/resource/image/0e/99/0ecb6d11e5e7725107c0291c45aa7e99.png)

### 怎么缓解 DDoS 攻击带来的性能下降问题？

* DoS（Denail of Service）：即拒绝服务攻击，指利用大量的合理请求，来占用过多的目标资源，从而使目标服务无法响应正常请求

* DDoS（Distributed Denial of Service）：是在 DoS 的基础上，采用了分布式架构，利用多台主机同时攻击目标主机。即使目标服务部署了网络防御设备，面对大量网络请求时，还是无力应对

从攻击的原理上来看，DDoS 可以分为下面几种类型：

* 第一种，耗尽带宽。无论是服务器还是路由器、交换机等网络设备，带宽都有固定的上限。带宽耗尽后，就会发生网络拥堵，从而无法传输其他正常的网络报文
* 第二种，耗尽操作系统的资源。网络服务的正常运行，都需要一定的系统资源，像是 CPU、内存等物理资源，以及连接表等软件资源。一旦资源耗尽，系统就不能处理其他正常的网络连接
* 第三种，消耗应用程序的运行资源。应用程序的运行，通常还需要跟其他的资源或系统交互。如果应用程序一直忙于处理无效请求，也会导致正常请求的处理变慢，甚至得不到响应

##### 案例分析

```
# 终端一 运行Nginx服务并对外开放80端口
# --network=host表示使用主机网络（这是为了方便后面排查问题）
$ docker run -itd --name=nginx --network=host nginx

# 终端二和终端三中，使用 curl 访问 Nginx 监听的端口，-w表示只输出HTTP状态码及总时间，-o表示将响应重定向到/dev/null
$ curl -s -w 'Http code: %{http_code}\nTotal time:%{time_total}s\n' -o /dev/null http://192.168.0.30/
...
Http code: 200
Total time:0.002s

正常情况下，访问 Nginx 只需要 2ms（0.002s）

# 在终端二中，运行 hping3 命令，来模拟 DoS 攻击
# -S参数表示设置TCP协议的SYN（同步序列号），-p表示目的端口为80
# -i u10表示每隔10微秒发送一个网络帧
$ hping3 -S -p 80 -i u10 192.168.0.30

回到终端一，现在不管执行什么命令，都慢了很多。不过，如果现象不那么明显，尝试把参数里面的 u10 调小（比如调成 u1），或者加上–flood 选项

# 终端三中，执行下面的命令，模拟正常客户端的连接
# --connect-timeout表示连接超时时间
$ curl -w 'Http code: %{http_code}\nTotal time:%{time_total}s\n' -o /dev/null --connect-timeout 10 http://192.168.0.30
...
Http code: 000
Total time:10.001s
curl: (28) Connection timed out after 10000 milliseconds

正常客户端的连接超时了，并没有收到 Nginx 服务的响应

# 回到终端一中，可以观察 PPS（每秒收发的报文数），BPS（每秒收发的字节数）
$ sar -n DEV 1
08:55:49        IFACE   rxpck/s   txpck/s    rxkB/s    txkB/s   rxcmp/s   txcmp/s  rxmcst/s   %ifutil
08:55:50      docker0      0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00
08:55:50         eth0  22274.00    629.00   1174.64     37.78      0.00      0.00      0.00      0.02
08:55:50           lo      0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00

网络接收的 PPS 已经达到了 20000 多，BPS 却只有 1174 kB，每个包的大小就只有 54B（1174*1024/22274=54）

# 终端一中，执行下面的 tcpdump 命令
# -i eth0 只抓取eth0网卡，-n不解析协议名和主机名
# tcp port 80表示只抓取tcp协议并且端口号为80的网络帧
$ tcpdump -i eth0 -n tcp port 80
09:15:48.287047 IP 192.168.0.2.27095 > 192.168.0.30: Flags [S], seq 1288268370, win 512, length 0
09:15:48.287050 IP 192.168.0.2.27131 > 192.168.0.30: Flags [S], seq 2084255254, win 512, length 0
09:15:48.287052 IP 192.168.0.2.27116 > 192.168.0.30: Flags [S], seq 677393791, win 512, length 0
09:15:48.287055 IP 192.168.0.2.27141 > 192.168.0.30: Flags [S], seq 1276451587, win 512, length 0
09:15:48.287068 IP 192.168.0.2.27154 > 192.168.0.30: Flags [S], seq 1851495339, win 512, length 0
...

Flags [S] 表示这是一个 SYN 包。大量的 SYN 包表明，这是一个 SYN Flood 攻击。用 Wireshark 来观察
```

![img](https://static001.geekbang.org/resource/image/f3/13/f397305c87be6ae43e065d3262ec9113.png)

SYN Flood  正是互联网中最经典的 DDoS 攻击方式。从上面这个图，可以看到它的原理：

* 即客户端构造大量的 SYN 包，请求建立 TCP 连接
* 而服务器收到包后，会向源 IP 发送 SYN+ACK 报文，并等待三次握手的最后一次 ACK 报文，直到超时

这种等待状态的 TCP 连接，通常也称为半开连接。由于连接表的大小有限，大量的半开连接就会导致连接表迅速占满，从而无法建立新的 TCP 连接。此时，服务器端的 TCP 连接，会处于 SYN_RECEIVED 状态：

![img](https://static001.geekbang.org/resource/image/86/a2/86dabf9cc66b29133fa6a239cfee38a2.png)

查看 TCP 半开连接的方法，关键在于 SYN_RECEIVED 状态的连接。可以使用 netstat ，来查看所有连接的状态

```
# 终端一中，执行下面的 netstat 命令
# -n表示不解析名字，-p表示显示连接所属进程
$ netstat -n -p | grep SYN_REC
tcp        0      0 192.168.0.30:80          192.168.0.2:12503      SYN_RECV    -
tcp        0      0 192.168.0.30:80          192.168.0.2:13502      SYN_RECV    -
tcp        0      0 192.168.0.30:80          192.168.0.2:15256      SYN_RECV    -
tcp        0      0 192.168.0.30:80          192.168.0.2:18117      SYN_RECV    -
...

大量 SYN_RECV 状态的连接，并且源 IP 地址为 192.168.0.2

# 统计所有 SYN_RECV 状态的连接数
$ netstat -n -p | grep SYN_REC | wc -l
193

要解决 SYN 攻击的问题，只要丢掉相关的包就可以。可以在终端一中，执行下面的 iptables 命令
$ iptables -I INPUT -s 192.168.0.2 -p tcp -j REJECT

# 回到终端三中，再次执行 curl 命令，查看正常用户访问 Nginx 的情况
$ curl -w 'Http code: %{http_code}\nTotal time:%{time_total}s\n' -o /dev/null --connect-timeout 10 http://192.168.0.30
Http code: 200
Total time:1.572171s

可以访问 Nginx 了，只是响应比较慢，从原来的 2ms 变成了现在的 1.5s，一般来说，SYN Flood 攻击中的源 IP 并不是固定的。可以在 hping3 命令中，加入 --rand-source 选项，来随机化源 IP

可以用以下两种方法，来限制 syn 包的速率

# 限制syn并发数为每秒1次
$ iptables -A INPUT -p tcp --syn -m limit --limit 1/s -j ACCEPT
# 限制单个IP在60秒新建立的连接数为10
$ iptables -I INPUT -p tcp --dport 80 --syn -m recent --name SYN_FLOOD --update --seconds 60 --hitcount 10 -j REJECT

如果是多台机器同时发送 SYN Flood，这种方法可能就无效了，需要事先对系统做一些 TCP 优化
# 默认的半连接容量只有 256
$ sysctl net.ipv4.tcp_max_syn_backlog
net.ipv4.tcp_max_syn_backlog = 256

# 将其增大为 1024
$ sysctl -w net.ipv4.tcp_max_syn_backlog=1024
net.ipv4.tcp_max_syn_backlog = 1024

# 连接每个 SYN_RECV 时，如果失败的话，内核还会自动重试，并且默认的重试次数是 5 次。可将其减小为 1 次
$ sysctl -w net.ipv4.tcp_synack_retries=1
net.ipv4.tcp_synack_retries = 1
```

TCP SYN Cookies 也是一种专门防御 SYN Flood 攻击的方法。SYN Cookies 基于连接信息（包括源地址、源端口、目的地址、目的端口等）以及一个加密种子（如系统启动时间），计算出一个哈希值（SHA1），这个哈希值称为 cookie。然后，这个 cookie 就被用作序列号，来应答 SYN+ACK 包，并释放连接状态。当客户端发送完三次握手的最后一次 ACK 后，服务器就会再次计算这个哈希值，确认是上次返回的 SYN+ACK 的返回包，才会进入 TCP 的连接状态，开启 TCP syncookies 后，内核选项 net.ipv4.tcp_max_syn_backlog 也就无效了

开启 TCP SYN Cookies：

```
$ sysctl -w net.ipv4.tcp_syncookies=1
net.ipv4.tcp_syncookies = 1

# 为了保证配置持久化，你还应该把这些配置，写入 /etc/sysctl.conf 文件中
$ cat /etc/sysctl.conf
net.ipv4.tcp_syncookies = 1
net.ipv4.tcp_synack_retries = 1
net.ipv4.tcp_max_syn_backlog = 1024

写入 /etc/sysctl.conf 的配置，需要执行 sysctl -p 命令后，才会动态生效
```

##### DDoS 到底该怎么防御

实际上，当 DDoS 报文到达服务器后，Linux 提供的机制只能缓解，而无法彻底解决。即使像是 SYN Flood 这样的小包攻击，其巨大的 PPS ，也会导致 Linux 内核消耗大量资源，进而导致其他网络报文的处理缓慢。虽然可以调整内核参数，缓解 DDoS 带来的性能问题，却也无法彻底解决它。Linux 内核中冗长的协议栈，在 PPS 很大时，就是一个巨大的负担。对 DDoS 攻击来说，也是一样的道理。所以，当时提到的 C10M 的方法，用到这里同样适合。比如，可以基于 XDP 或者 DPDK，构建 DDoS 方案，在内核网络协议栈前，或者跳过内核协议栈，来识别并丢弃 DDoS 报文，避免 DDoS 对系统其他资源的消耗。

因为 DDoS 并不一定是因为大流量或者大 PPS，有时候，慢速的请求也会带来巨大的性能下降（这种情况称为慢速 DDoS）。比如，很多针对应用程序的攻击，都会伪装成正常用户来请求资源。这种情况下，请求流量可能本身并不大，但响应流量却可能很大，并且应用程序内部也很可能要耗费大量资源处理。这时，就需要应用程序考虑识别，并尽早拒绝掉这些恶意流量，比如合理利用缓存、增加 WAF（Web Application Firewall）、使用 CDN 等等

### 网络请求延迟变大了，该怎么办？

除了 DDoS 会带来网络延迟增大外，其他原因导致的网络延迟，比如：

* 网络传输慢
* Linux 内核协议栈报文处理慢
* 应用程序数据处理慢

##### 网络延迟

* 网络延迟：网络数据传输所用的时间。这个时间可能是单向的，指从源地址发送到目的地址的单程时间；也可能是双向的，即从源地址发送到目的地址，然后又从目的地址发回响应，这个往返全程所用的时间。通常，更常用的是双向的往返通信延迟，比如 ping 测试的结果，就是往返延时 RTT（Round-Trip Time）

* 应用程序延迟：从应用程序接收到请求，再到发回响应，全程所用的时间。通常，应用程序延迟也指的是往返延迟，是网络数据传输时间加上数据处理时间的和

ping 基于 ICMP 协议，它通过计算 ICMP 回显响应报文与 ICMP 回显请求报文的时间差，来获得往返延时。这个过程并不需要特殊认证，常被很多网络攻击利用，比如端口扫描工具 nmap、组包工具 hping3 等等。为了避免这些问题，很多网络服务会把 ICMP 禁止掉，导致无法用 ping 来测试网络服务的可用性和往返延时。这时可以用 traceroute 或 hping3 的 TCP 和 UDP 模式，来获取网络延迟

```
# 执行下面的 hping3 命令，测试你的机器到百度搜索服务器的网络延迟
# -c表示发送3次请求，-S表示设置TCP SYN，-p表示端口号为80
$ hping3 -c 3 -S -p 80 baidu.com
HPING baidu.com (eth0 123.125.115.110): S set, 40 headers + 0 data bytes
len=46 ip=123.125.115.110 ttl=51 id=47908 sport=80 flags=SA seq=0 win=8192 rtt=20.9 ms
len=46 ip=123.125.115.110 ttl=51 id=6788  sport=80 flags=SA seq=1 win=8192 rtt=20.9 ms
len=46 ip=123.125.115.110 ttl=51 id=37699 sport=80 flags=SA seq=2 win=8192 rtt=20.9 ms

--- baidu.com hping statistic ---
3 packets transmitted, 3 packets received, 0% packet loss
round-trip min/avg/max = 20.9/20.9/20.9 ms

往返延迟 RTT 为 20.9ms

# 用 traceroute ，也可以得到类似结果
# --tcp表示使用TCP协议，-p表示端口号，-n表示不对结果中的IP地址执行反向域名解析
$ traceroute --tcp -p 80 -n baidu.com
traceroute to baidu.com (123.125.115.110), 30 hops max, 60 byte packets
 1  * * *
 2  * * *
 3  * * *
 4  * * *
 5  * * *
 6  * * *
 7  * * *
 8  * * *
 9  * * *
10  * * *
11  * * *
12  * * *
13  * * *
14  123.125.115.110  20.684 ms *  20.798 ms

traceroute 会在路由的每一跳发送三个包，并在收到响应后，输出往返延时。如果无响应或者响应超时（默认 5s），就会输出一个星号
```

网络延迟，是最核心的网络性能指标。由于网络传输、网络包处理等各种因素的影响，网络延迟不可避免。但过大的网络延迟，会直接影响用户的体验。所以，在发现网络延迟增大后，需要定位网络中的潜在问题。比如：

* 使用 hping3 以及 wrk 等工具，确认单次请求和并发请求情况的网络延迟是否正常
* 使用 traceroute，确认路由是否正确，并查看路由中每一跳网关的延迟
* 使用 tcpdump 和 Wireshark，确认网络包的收发是否正常
* 使用 strace 等，观察应用程序对网络套接字的调用情况是否正常

这样，就可以依次从路由、网络包的收发、再到应用程序等，逐层排查，直到定位问题根源

### 如何优化 NAT 性能？

另一个可能导致网络延迟的因素，即网络地址转换（Network Address Translation），缩写为 NAT

##### NAT 原理

NAT 技术可以重写 IP 数据包的源 IP 或者目的 IP，被普遍地用来解决公网 IP 地址短缺的问题。主要原理是网络中的多台主机，通过共享同一个公网 IP 地址，来访问外网资源。同时，由于 NAT 屏蔽了内网网络，自然也就为局域网中的机器提供了安全隔离。既可以在支持网络地址转换的路由器（称为 NAT 网关）中配置 NAT，也可以在 Linux 服务器中配置 NAT。如果采用第二种方式，Linux 服务器实际上充当的是“软”路由器的角色。NAT 的主要目的，是实现地址转换。根据实现方式的不同，NAT 可以分为三类：

* 静态 NAT：即内网 IP 与公网 IP 是一对一的永久映射关系
* 动态 NAT：即内网 IP 从公网 IP 池中，动态选择一个进行映射
* 网络地址端口转换 NAPT（Network Address and Port Translation）：即把内网 IP 映射到公网 IP 的不同端口上，让多个内网 IP 可以共享同一个公网 IP 地址。是目前最流行的 NAT 类型，在 Linux 中配置的 NAT 也是这种类型。而根据转换方式的不同，又可以把 NAPT 分为三类
  * 第一类是源地址转换 SNAT：即目的地址不变，只替换源 IP 或源端口。SNAT 主要用于，多个内网 IP 共享同一个公网 IP ，来访问外网资源的场景
  * 第二类是目的地址转换 DNAT：即源 IP 保持不变，只替换目的 IP 或者目的端口。DNAT 主要通过公网 IP 的不同端口号，来访问内网的多种服务，同时会隐藏后端服务器的真实 IP 地址
  * 第三类是双向地址转换：即同时使用 SNAT 和 DNAT。当接收到网络包时，执行 DNAT，把目的 IP 转换为内网 IP；而在发送网络包时，执行 SNAT，把源 IP 替换为外部 IP

![img](https://static001.geekbang.org/resource/image/c7/e4/c743105dc7bd955a4a300d6b55b7a0e4.png)

##### iptables 与 NAT

Linux 内核提供的 Netfilter 框架，允许对网络数据包进行修改（比如 NAT）和过滤（比如防火墙）。iptables、ip6tables、ebtables 等工具，又提供了更易用的命令行接口，以便系统管理员配置和管理 NAT、防火墙的规则。其中，iptables 就是最常用的一种配置工具。要掌握 iptables 的原理和使用方法，最核心的就是弄清楚，网络数据包通过 Netfilter 时的工作流向

![img](https://static001.geekbang.org/resource/image/c6/56/c6de40c5bd304132a1b508ba669e7b56.png)

绿色背景的方框，表示表（table），用来管理链。Linux 支持 4 种表，包括 filter（用于过滤）、nat（用于 NAT）、mangle（用于修改分组数据） 和 raw（用于原始数据包）等。跟 table 一起的白色背景方框，则表示链（chain），用来管理具体的 iptables 规则。每个表中可以包含多条链，比如：

* filter 表中，内置 INPUT、OUTPUT 和 FORWARD 链
* nat 表中，内置 PREROUTING、POSTROUTING、OUTPUT 等

要实现 NAT 功能，主要是在 nat 表进行操作。而 nat 表内置了三个链：

* PREROUTING，用于路由判断前所执行的规则，比如，对接收到的数据包进行 DNAT
* POSTROUTING，用于路由判断后所执行的规则，比如，对发送或转发的数据包进行 SNAT 或 MASQUERADE
* OUTPUT，类似于 PREROUTING，但只处理从本机发送出去的包

##### SNAT

SNAT 需要在 nat 表的 POSTROUTING 链中配置。常用两种方式来配置它

第一种方法，是为一个子网统一配置 SNAT，并由 Linux 选择默认的出口 IP。就是 MASQUERADE：

```
$ iptables -t nat -A POSTROUTING -s 192.168.0.0/16 -j MASQUERADE
```

第二种方法，是为具体的 IP 地址配置 SNAT，并指定转换后的源地址：

```
$ iptables -t nat -A POSTROUTING -s 192.168.0.2 -j SNAT --to-source 100.100.100.100
```

##### DNAT

DNAT 需要在 nat 表的 PREROUTING 或者 OUTPUT 链中配置，其中， PREROUTING 链更常用一些

```
$ iptables -t nat -A PREROUTING -d 100.100.100.100 -j DNAT --to-destination 192.168.0.2
```

##### 双向地址转换

就是同时添加 SNAT 和 DNAT 规则，为公网 IP 和内网 IP 实现一对一的映射关系，即：

```
$ iptables -t nat -A POSTROUTING -s 192.168.0.2 -j SNAT --to-source 100.100.100.100
$ iptables -t nat -A PREROUTING -d 100.100.100.100 -j DNAT --to-destination 192.168.0.2
```

使用 iptables 配置 NAT 规则时，Linux 需要转发来自其他 IP 的网络包，所以千万不要忘记开启 Linux 的 IP 转发功能。可以执行下面的命令，查看这一功能是否开启。如果输出的结果是 1，就表示已经开启了 IP 转发：

```
$ sysctl net.ipv4.ip_forward
net.ipv4.ip_forward = 1
```

如果还没开启，你可以执行下面的命令，手动开启：

```
$ sysctl -w net.ipv4.ip_forward=1
net.ipv4.ip_forward = 1
```

为了避免重启后配置丢失，不要忘记将配置写入 /etc/sysctl.conf 文件中

分析工具：

* SystemTap：Linux 的一种动态追踪框架，它把用户提供的脚本，转换为内核模块来执行，用来监测和跟踪内核的行为

```
# Ubuntu
apt-get install -y systemtap-runtime systemtap
# Configure ddebs source
echo "deb http://ddebs.ubuntu.com $(lsb_release -cs) main restricted universe multiverse
deb http://ddebs.ubuntu.com $(lsb_release -cs)-updates main restricted universe multiverse
deb http://ddebs.ubuntu.com $(lsb_release -cs)-proposed main restricted universe multiverse" | \
sudo tee -a /etc/apt/sources.list.d/ddebs.list
# Install dbgsym
apt-key adv --keyserver keyserver.ubuntu.com --recv-keys F2EDC64DC5AEE1F6B9C621F0C8CAB6595FDFF622
apt-get update
apt install ubuntu-dbgsym-keyring
stap-prep
apt-get install linux-image-`uname -r`-dbgsym
```

首先运行一个不用 NAT 的 Nginx 服务，并用 ab 测试它的性能

```
# 在终端一中，执行下面的命令，启动 Nginx，选项 --network=host ，表示容器使用 Host 网络模式，即不使用 NAT
$ docker run --name nginx-hostnet --privileged --network=host -itd feisky/nginx:80

# 终端二中，执行 curl 命令，确认 Nginx 正常启动
$ curl http://192.168.0.30/

# 在终端二中，执行 ab 命令，对 Nginx 进行压力测试。Linux 默认允许打开的文件描述数比较小，比如默认只有 1024
$ ulimit -n
1024

# 临时增大当前会话的最大文件描述符数
$ ulimit -n 65536

# -c表示并发请求数为5000，-n表示总的请求数为10万
# -r表示套接字接收错误时仍然继续执行，-s表示设置每个请求的超时时间为2s
$ ab -c 5000 -n 100000 -r -s 2 http://192.168.0.30/
...
Requests per second:    6576.21 [#/sec] (mean)
Time per request:       760.317 [ms] (mean)
Time per request:       0.152 [ms] (mean, across all concurrent requests)
Transfer rate:          5390.19 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0  177 714.3      9    7338
Processing:     0   27  39.8     19     961
Waiting:        0   23  39.5     16     951
Total:          1  204 716.3     28    7349
...

每秒请求数（Requests  per second）为 6576；每个请求的平均延迟（Time per request）为 760ms；建立连接的平均延迟（Connect）为 177ms。这几个数值，这将是接下来案例的基准指标

# 回到终端一，停止这个未使用 NAT 的 Nginx 应用：
$ docker rm -f nginx-hostnet
# 使用了 DNAT ，来实现 Host 的 8080 端口，到容器的 8080 端口的映射关系
$ docker run --name nginx --privileged -p 8080:8080 -itd feisky/nginx:nat

# 可以执行 iptables 命令，确认 DNAT 规则已经创建
$ iptables -nL -t nat
Chain PREROUTING (policy ACCEPT)
target     prot opt source               destination
DOCKER     all  --  0.0.0.0/0            0.0.0.0/0            ADDRTYPE match dst-type LOCAL

...

Chain DOCKER (2 references)
target     prot opt source               destination
RETURN     all  --  0.0.0.0/0            0.0.0.0/0
DNAT       tcp  --  0.0.0.0/0            0.0.0.0/0            tcp dpt:8080 to:172.17.0.2:8080

在 PREROUTING 链中，目的为本地的请求，会转到 DOCKER 链；而在 DOCKER 链中，目的端口为 8080 的 tcp 请求，会被 DNAT 到 172.17.0.2 的 8080 端口。其中，172.17.0.2 就是 Nginx 容器的 IP 地址

# 切换到终端二中，执行 curl 命令，确认 Nginx 已经正常启动
$ curl http://192.168.0.30:8080/
# 再次执行上述的 ab 命令，不过这次注意，要把请求的端口号换成 8080
# -c表示并发请求数为5000，-n表示总的请求数为10万
# -r表示套接字接收错误时仍然继续执行，-s表示设置每个请求的超时时间为2s
$ ab -c 5000 -n 100000 -r -s 2 http://192.168.0.30:8080/
...
apr_pollset_poll: The timeout specified has expired (70007)
Total of 5602 requests completed

刚才正常运行的 ab ，现在失败了，还报了连接超时的错误。运行 ab 时的 -s 参数，设置了每个请求的超时时间为 2s，而从输出可以看到，这次只完成了 5602 个请求

# 不妨把超时时间延长到 30s。意味着要等更长时间，为了快点得到结果，同时把总测试次数，也减少到 10000:
$ ab -c 5000 -n 10000 -r -s 30 http://192.168.0.30:8080/
...
Requests per second:    76.47 [#/sec] (mean)
Time per request:       65380.868 [ms] (mean)
Time per request:       13.076 [ms] (mean, across all concurrent requests)
Transfer rate:          44.79 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0 1300 5578.0      1   65184
Processing:     0 37916 59283.2      1  130682
Waiting:        0    2   8.7      1     414
Total:          1 39216 58711.6   1021  130682
...

每秒请求数（Requests per second）为 76；每个请求的延迟（Time per request）为 65s；建立连接的延迟（Connect）为 1300ms。显然，每个指标都比前面差了很多
知道 NAT 是罪魁祸首。所以，有理由怀疑，内核中发生了丢包现象

# 回到终端一中，创建一个 dropwatch.stp 的脚本文件，并写入下面的内容
#! /usr/bin/env stap

############################################################
# Dropwatch.stp
# Author: Neil Horman <nhorman@redhat.com>
# An example script to mimic the behavior of the dropwatch utility
# http://fedorahosted.org/dropwatch
############################################################

# Array to hold the list of drop points we find
global locations

# Note when we turn the monitor on and off
probe begin { printf("Monitoring for dropped packets\n") }
probe end { printf("Stopping dropped packet monitor\n") }

# increment a drop counter for every location we drop at
probe kernel.trace("kfree_skb") { locations[$location] <<< 1 }

# Every 5 seconds report our drop locations
probe timer.sec(5)
{
  printf("\n")
  foreach (l in locations-) {
    printf("%d packets dropped at %s\n",
           @count(locations[l]), symname(l))
  }
  delete locations
}

这个脚本，跟踪内核函数 kfree_skb() 的调用，并统计丢包的位置。文件保存好后，执行下面的 stap 命令，就可以运行丢包跟踪脚本。stap，是 SystemTap 的命令行工具
$ stap --all-modules dropwatch.stp
Monitoring for dropped packets

# 切换到终端二中，再次执行 ab 命令
$ ab -c 5000 -n 10000 -r -s 30 http://192.168.0.30:8080/

# 再次回到终端一中，观察 stap 命令的输出
10031 packets dropped at nf_hook_slow
676 packets dropped at tcp_v4_rcv

7284 packets dropped at nf_hook_slow
268 packets dropped at tcp_v4_rcv

大量丢包都发生在 nf_hook_slow 位置。这是在 Netfilter Hook 的钩子函数中，出现丢包问题了。但是不是 NAT，还不能确定。还得再跟踪  nf_hook_slow 的执行过程，这一步可以通过 perf 来完成

# 切换到终端二中，再次执行 ab 命令
$ ab -c 5000 -n 10000 -r -s 30 http://192.168.0.30:8080/
# 再次切换回终端一，执行 perf record 和 perf report 命令
# 记录一会（比如30s）后按Ctrl+C结束
$ perf record -a -g -- sleep 30
# 输出报告
$ perf report -g graph,0

在 perf report 界面中，输入查找命令 / 然后，在弹出的对话框中，输入 nf_hook_slow，发现
nf_hook_slow 调用最多的有三个地方，分别是 ipv4_conntrack_in、br_nf_pre_routing 以及 iptable_nat_ipv4_in。换言之，nf_hook_slow 主要在执行三个动作
第一，接收网络包时，在连接跟踪表中查找连接，并为新的连接分配跟踪对象（Bucket）
第二，在 Linux 网桥中转发包。这是因为案例 Nginx 是一个 Docker 容器，而容器的网络通过网桥来实现
第三，接收网络包时，执行 DNAT，即把 8080 端口收到的包转发给容器

DNAT 的基础是 conntrack，所以先看看，内核提供了哪些 conntrack 的配置选项。在终端一中，继续执行下面的命令
$ sysctl -a | grep conntrack
net.netfilter.nf_conntrack_count = 180
net.netfilter.nf_conntrack_max = 1000
net.netfilter.nf_conntrack_buckets = 65536
net.netfilter.nf_conntrack_tcp_timeout_syn_recv = 60
net.netfilter.nf_conntrack_tcp_timeout_syn_sent = 120
net.netfilter.nf_conntrack_tcp_timeout_time_wait = 120
...

net.netfilter.nf_conntrack_count，表示当前连接跟踪数
net.netfilter.nf_conntrack_max，表示最大连接跟踪数
net.netfilter.nf_conntrack_buckets，表示连接跟踪表的大小

当前连接跟踪数是 180，最大连接跟踪数是 1000，连接跟踪表的大小，则是 65536。回想一下前面的 ab 命令，并发请求数是 5000，而请求数是 100000。显然，跟踪表设置成，只记录 1000 个连接，是远远不够的

# 内核在工作异常时，会把异常信息记录到日志中
$ dmesg | tail
[104235.156774] nf_conntrack: nf_conntrack: table full, dropping packet
[104243.800401] net_ratelimit: 3939 callbacks suppressed
[104243.800401] nf_conntrack: nf_conntrack: table full, dropping packet
[104262.962157] nf_conntrack: nf_conntrack: table full, dropping packet

net_ratelimit 表示有大量的日志被压缩掉了，这是内核预防日志攻击的一种措施。而当你看到 “nf_conntrack: table full” 的错误时，就表明 nf_conntrack_max 太小了。nf_conntrack_buckets，就是哈希表的大小。哈希表中的每一项，都是一个链表（称为 Bucket），而链表长度，就等于 nf_conntrack_max 除以 nf_conntrack_buckets。比如，可以估算一下，上述配置的连接跟踪表占用的内存大小

# 连接跟踪对象大小为376，链表项大小为16
nf_conntrack_max*连接跟踪对象大小+nf_conntrack_buckets*链表项大小 
= 1000*376+65536*16 B
= 1.4 MB

# 将 nf_conntrack_max 改大一些，比如改成 131072（即 nf_conntrack_buckets 的 2 倍）
$ sysctl -w net.netfilter.nf_conntrack_max=131072
$ sysctl -w net.netfilter.nf_conntrack_buckets=65536

# 再切换到终端二中，重新执行 ab 命令。注意，这次我们把超时时间也改回原来的 2s
$ ab -c 5000 -n 100000 -r -s 2 http://192.168.0.30:8080/
...
Requests per second:    6315.99 [#/sec] (mean)
Time per request:       791.641 [ms] (mean)
Time per request:       0.158 [ms] (mean, across all concurrent requests)
Transfer rate:          4985.15 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0  355 793.7     29    7352
Processing:     8  311 855.9     51   14481
Waiting:        0  292 851.5     36   14481
Total:         15  666 1216.3    148   14645

每秒请求数（Requests per second）为 6315（不用 NAT 时为 6576）
每个请求的延迟（Time per request）为 791ms（不用 NAT 时为 760ms）
建立连接的延迟（Connect）为 355ms（不用 NAT 时为 177ms）

# 可以用 conntrack 命令行工具，来查看连接跟踪表的内容
# -L表示列表，-o表示以扩展格式显示
$ conntrack -L -o extended | head
ipv4     2 tcp      6 7 TIME_WAIT src=192.168.0.2 dst=192.168.0.96 sport=51744 dport=8080 src=172.17.0.2 dst=192.168.0.2 sport=8080 dport=51744 [ASSURED] mark=0 use=1
ipv4     2 tcp      6 6 TIME_WAIT src=192.168.0.2 dst=192.168.0.96 sport=51524 dport=8080 src=172.17.0.2 dst=192.168.0.2 sport=8080 dport=51524 [ASSURED] mark=0 use=1

# 在终端二启动 ab 命令后，再回到终端一中，执行下面的命令
# 统计总的连接跟踪数
$ conntrack -L -o extended | wc -l
14289

# 统计TCP协议各个状态的连接跟踪数
$ conntrack -L -o extended | awk '/^.*tcp.*$/ {sum[$6]++} END {for(i in sum) print i, sum[i]}'
SYN_RECV 4
CLOSE_WAIT 9
ESTABLISHED 2877
FIN_WAIT 3
SYN_SENT 2113
TIME_WAIT 9283

# 统计各个源IP的连接跟踪数
$ conntrack -L -o extended | awk '{print $7}' | cut -d "=" -f 2 | sort | uniq -c | sort -nr | head -n 10
  14116 192.168.0.2
    172 192.168.0.96
```

### 网络性能优化的几个思路

##### 确定优化目标

虽然网络性能优化的整体目标，是降低网络延迟（如 RTT）和提高吞吐量（如 BPS 和 PPS），但具体到不同应用中，每个指标的优化标准可能会不同，优先级顺序也大相径庭

* NAT 网关来说，由于其直接影响整个数据中心的网络出入性能，所以通常需要达到或接近线性转发，也就是说， PPS 是最主要的性能目标
* 对于数据库、缓存等系统，快速完成网络收发，即低延迟，是主要的性能目标
* 对于经常访问的 Web 服务来说，则需要同时兼顾吞吐量和延迟

为了更客观合理地评估优化效果，首先应该明确优化的标准，即要对系统和应用程序进行基准测试，得到网络协议栈各层的基准性能

![img](https://static001.geekbang.org/resource/image/c7/ac/c7b5b16539f90caabb537362ee7c27ac.png)

底层是其上方各层的基础，底层性能也就决定了高层性能。底层性能指标，其实就是对应高层的极限性能

* 首先是网络接口层和网络层，它们主要负责网络包的封装、寻址、路由，以及发送和接收。每秒可处理的网络包数 PPS，就是它们最重要的性能指标（特别是在小包的情况下）。可以用内核自带的发包工具 pktgen ，来测试 PPS 的性能
* 再向上到传输层的 TCP 和 UDP，它们主要负责网络传输。对它们而言，吞吐量（BPS）、连接数以及延迟，就是最重要的性能指标。可以用 iperf 或 netperf ，来测试传输层的性能。网络包的大小，会直接影响这些指标的值。所以，通常需要测试一系列不同大小网络包的性能。
* 再往上到了应用层，最需要关注的是吞吐量（BPS）、每秒请求数以及延迟等指标。可以用 wrk、ab 等工具，来测试应用程序的性能

##### 网络性能工具

![img](https://static001.geekbang.org/resource/image/a1/3b/a1eb07e281e5795be83c11d7255c543b.png)

![img](https://static001.geekbang.org/resource/image/0d/a0/0d87b39b89a1b7f325fc5477c0182ea0.png)

##### 网络性能优化

总的来说，先要获得网络基准测试报告，然后通过相关性能工具，定位出网络性能瓶颈。再接下来的优化工作，就是水到渠成，要优化网络性能，肯定离不开 Linux 系统的网络协议栈和网络收发流程的辅助

![img](https://static001.geekbang.org/resource/image/a1/3f/a118911721f9b67ce9c83de15666753f.png)

##### 应用程序

通常通过套接字接口进行网络操作。由于网络收发通常比较耗时，所以应用程序的优化，主要就是对网络 I/O 和进程自身的工作模型的优化

从网络 I/O 的角度来说，主要有下面两种优化思路

* 第一种是最常用的 I/O 多路复用技术 epoll，主要用来取代 select 和 poll。这其实是解决 C10K 问题的关键，也是目前很多网络应用默认使用的机制
* 第二种是使用异步 I/O（Asynchronous I/O，AIO），使用比较复杂，需要小心处理很多边缘情况

而从进程的工作模型来说，也有两种不同的模型用来优化

* 第一种，主进程 + 多个 worker 子进程。其中，主进程负责管理网络连接，而子进程负责实际的业务处理。这也是最常用的一种模型
* 第二种，监听到相同端口的多进程模型。在这种模型下，所有进程都会监听相同接口，并且开启 SO_REUSEPORT 选项，由内核负责，把请求负载均衡到这些监听进程中去

应用层的网络协议优化：

* 使用长连接取代短连接，可以显著降低 TCP 建立连接的成本。在每秒请求次数较多时，效果非常明显
* 使用内存等方式，来缓存不常变化的数据，可以降低网络 I/O 次数，同时加快应用程序的响应速度
* 使用 Protocol Buffer 等序列化的方式，压缩网络 I/O 的数据量，可以提高应用程序的吞吐
* 使用 DNS 缓存、预取、HTTPDNS 等方式，减少 DNS 解析的延迟，也可以提升网络 I/O 的整体速度

##### 套接字

套接字可以屏蔽掉 Linux 内核中不同协议的差异，为应用程序提供统一的访问接口。每个套接字，都有一个读写缓冲区，为了提高网络的吞吐量，通常需要调整这些缓冲区的大小。比如：

* 增大每个套接字的缓冲区大小 net.core.optmem_max
* 增大套接字接收缓冲区大小 net.core.rmem_max 和发送缓冲区大小 net.core.wmem_max
* 增大 TCP 接收缓冲区大小 net.ipv4.tcp_rmem 和发送缓冲区大小 net.ipv4.tcp_wmem

![img](https://static001.geekbang.org/resource/image/5f/f0/5f2d4957663dd8bf3410da8180ab18f0.png)

tcp_rmem 和 tcp_wmem 的三个数值分别是 min，default，max，系统会根据这些设置，自动调整 TCP 接收 / 发送缓冲区的大小。udp_mem 的三个数值分别是 min，pressure，max，系统会根据这些设置，自动调整 UDP 发送缓冲区的大小。表格中的数值只提供参考价值，具体应该设置多少，还需要根据实际的网络状况来确定。比如，发送缓冲区大小，理想数值是吞吐量 * 延迟，这样才可以达到最大网络利用率。除此之外，套接字接口还提供了一些配置选项，用来修改网络连接的行为：

* 为 TCP 连接设置 TCP_NODELAY 后，就可以禁用 Nagle 算法
* 为 TCP 连接开启 TCP_CORK 后，可以让小包聚合成大包后再发送（注意会阻塞小包的发送）
* 使用 SO_SNDBUF 和 SO_RCVBUF ，可以分别调整套接字发送缓冲区和接收缓冲区的大小

##### 网络性能优化

1. 传输层

传输层最重要的是 TCP 和 UDP 协议，其实主要就是对这两种协议的优化。TCP 提供了面向连接的可靠传输服务。要优化 TCP，掌握 TCP 协议的基本原理，比如流量控制、慢启动、拥塞避免、延迟确认以及状态流图

![img](https://static001.geekbang.org/resource/image/c0/d1/c072bb9c9dfd727ed187bc24beb3e3d1.png)

* 第一类，在请求数比较大的场景下，大量处于 TIME_WAIT 状态的连接，它们会占用大量内存和端口资源。可以优化与 TIME_WAIT 状态相关的内核选项，比如采取下面几种措施
  * 增大处于 TIME_WAIT 状态的连接数量 net.ipv4.tcp_max_tw_buckets ，并增大连接跟踪表的大小 net.netfilter.nf_conntrack_max
  * 减小 net.ipv4.tcp_fin_timeout 和 net.netfilter.nf_conntrack_tcp_timeout_time_wait ，让系统尽快释放它们所占用的资源
  * 开启端口复用 net.ipv4.tcp_tw_reuse。这样，被 TIME_WAIT 状态占用的端口，还能用到新建的连接中
  * 增大本地端口的范围 net.ipv4.ip_local_port_range 。这样就可以支持更多连接，提高整体的并发能力
  * 增加最大文件描述符的数量。使用 fs.nr_open 和 fs.file-max ，分别增大进程和系统的最大文件描述符数；或在应用程序的 systemd 配置文件中，配置 LimitNOFILE ，设置应用程序的最大文件描述符数

* 第二类，为了缓解 SYN FLOOD 等，利用 TCP 协议特点进行攻击而引发的性能问题，优化与 SYN 状态相关的内核选项，比如采取下面几种措施
  * 增大 TCP 半连接的最大数量 net.ipv4.tcp_max_syn_backlog ，或者开启 TCP SYN Cookies net.ipv4.tcp_syncookies ，来绕开半连接数量限制的问题（注意，这两个选项不可同时使用）
  * 减少 SYN_RECV 状态的连接重传 SYN+ACK 包的次数 net.ipv4.tcp_synack_retries

* 第三类，在长连接的场景中，通常使用 Keepalive 来检测 TCP 连接的状态，以便对端连接断开后，可以自动回收。系统默认的 Keepalive 探测间隔和重试次数，一般都无法满足应用程序的性能要求
  * 缩短最后一次数据包到 Keepalive 探测包的间隔时间 net.ipv4.tcp_keepalive_time
  * 缩短发送 Keepalive 探测包的间隔时间 net.ipv4.tcp_keepalive_intvl
  * 减少 Keepalive 探测失败后，一直到通知应用程序前的重试次数 net.ipv4.tcp_keepalive_probes

![img](https://static001.geekbang.org/resource/image/b0/e0/b07ea76a8737ed93395736795ede44e0.png)

如果同时使用不同优化方法，可能会产生冲突。比如，就像网络请求延迟的案例中我们曾经分析过的，服务器端开启 Nagle 算法，而客户端开启延迟确认机制，就很容易导致网络延迟增大。另外，在使用  NAT 的服务器上，如果开启 net.ipv4.tcp_tw_recycle ，就很容易导致各种连接失败。实际上，由于坑太多，这个选项在内核的 4.1 版本中已经删除了

UDP 提供了面向数据报的网络协议，它不需要网络连接，也不提供可靠性保障。UDP 优化，相对于 TCP 来说，要简单得多

* 增大套接字缓冲区大小以及 UDP 缓冲区范围
* 增大本地端口号的范围
* 根据 MTU 大小，调整 UDP 数据包的大小，减少或者避免分片的发生

2. 网络层

网络层，负责网络包的封装、寻址和路由，包括 IP、ICMP 等常见协议。在网络层，最主要的优化，其实就是对路由、 IP 分片以及 ICMP 等进行调优

* 第一种，从路由和转发的角度出发，可以调整下面的内核选项
  * 在需要转发的服务器中，比如用作 NAT 网关的服务器或者使用 Docker 容器时，开启 IP 转发，即设置 net.ipv4.ip_forward = 1
  * 调整数据包的生存周期 TTL，比如设置 net.ipv4.ip_default_ttl = 64。注意，增大该值会降低系统性能
  * 开启数据包的反向地址校验，比如设置 net.ipv4.conf.eth0.rp_filter = 1。这样可以防止 IP 欺骗，并减少伪造 IP 带来的 DDoS 问题

* 第二种，从分片的角度出发，最主要的是调整 MTU（Maximum Transmission Unit）的大小，通常，MTU 的大小应该根据以太网的标准来设置。以太网标准规定，一个网络帧最大为 1518B，那么去掉以太网头部的 18B 后，剩余的 1500 就是以太网 MTU 的大小，很多网络设备都支持巨帧，可以把 MTU 调大为 9000，以提高网络吞吐量

* 第三种，从 ICMP 的角度出发，为了避免 ICMP 主机探测、ICMP Flood 等各种网络问题
  * 禁止 ICMP 协议，即设置 net.ipv4.icmp_echo_ignore_all = 1。外部主机就无法通过 ICMP 来探测主机
  * 禁止广播 ICMP，即设置 net.ipv4.icmp_echo_ignore_broadcasts = 1

3. 链路层

链路层负责网络包在物理网络中的传输，比如 MAC 寻址、错误侦测以及通过网卡传输网络帧等

由于网卡收包后调用的中断处理程序（特别是软中断），需要消耗大量的 CPU。将这些中断处理程序调度到不同的 CPU 上执行，就可以显著提高网络吞吐量

* 为网卡硬中断配置 CPU 亲和性（smp_affinity），或者开启 irqbalance 服务
* 开启 RPS（Receive Packet Steering）和 RFS（Receive Flow Steering），将应用程序和软中断的处理，调度到相同 CPU 上，这样就可以增加 CPU 缓存命中率，减少网络延迟

现在的网卡都有很丰富的功能，原来在内核中通过软件处理的功能，可以卸载到网卡中，通过硬件来执行