# ALL

### 为什么应用容器化后，启动慢了很多？

随着 Kubernetes、Docker 等技术的普及，越来越多的企业，都已经走上了应用程序容器化的道路。基于 Docker 的微服务架构带来的各种优势，比如：

- 使用 Docker ，把应用程序以及相关依赖打包到镜像中后，部署和升级更快捷
- 把传统的单体应用拆分成多个更小的微服务应用后，每个微服务的功能都更简单，并且可以单独管理和维护
- 每个微服务都可以根据需求横向扩展。即使发生故障，也只是局部服务不可用，而不像以前那样，导致整个服务不可用

不过，任何技术都不是银弹。这些新技术，在带来诸多便捷功能之外，也带来了更高的复杂性，比如性能降低、架构复杂、排错困难等等

分析工具：

- jq：专门用来在命令行中处理 json。为了更好的展示 json 数据的输出

##### 案例分析

```
# 在终端二中执行下面的命令，多次尝试访问 Tomcat
$ for ((i=0;i<30;i++)); do curl localhost:8080; sleep 1; done
curl: (56) Recv failure: Connection reset by peer
curl: (56) Recv failure: Connection reset by peer
# 这儿会阻塞一会
Hello, wolrd!
curl: (52) Empty reply from server
curl: (7) Failed to connect to localhost port 8080: Connection refused
curl: (7) Failed to connect to localhost port 8080: Connection refused

# 终端一中
18-Feb-2019 12:43:32.719 INFO [localhost-startStop-1] org.apache.catalina.startup.HostConfig.deployDirectory Deploying web application directory [/usr/local/tomcat/webapps/docs]
18-Feb-2019 12:43:33.725 INFO [localhost-startStop-1] org.apache.catalina.startup.HostConfig.deployDirectory Deployment of web application directory [/usr/local/tomcat/webapps/docs] has finished in [1,006] ms
18-Feb-2019 12:43:33.726 INFO [localhost-startStop-1] org.apache.catalina.startup.HostConfig.deployDirectory Deploying web application directory [/usr/local/tomcat/webapps/manager]
18-Feb-2019 12:43:34.521 INFO [localhost-startStop-1] org.apache.catalina.startup.HostConfig.deployDirectory Deployment of web application directory [/usr/local/tomcat/webapps/manager] has finished in [795] ms
18-Feb-2019 12:43:34.722 INFO [main] org.apache.coyote.AbstractProtocol.start Starting ProtocolHandler ["http-nio-8080"]
18-Feb-2019 12:43:35.319 INFO [main] org.apache.coyote.AbstractProtocol.start Starting ProtocolHandler ["ajp-nio-8009"]
18-Feb-2019 12:43:35.821 INFO [main] org.apache.catalina.startup.Catalina.start Server startup in 24096 ms
root@ubuntu:~#

最后一行，明显是回到了 Linux 的 SHELL 终端中，而没有继续等待 Docker 输出的容器日志。上一个命令是 docker logs -f 命令。它的退出就只有两种可能了，要么是容器退出了，要么就是 dockerd 进程退出了

# 终端一中，执行下面的命令，查看容器的状态：容器已经退出
$ docker ps -a
CONTAINER ID        IMAGE               COMMAND             CREATED             STATUS                            PORTS               NAMES
0f2b3fcdd257        feisky/tomcat:8     "catalina.sh run"   2 minutes ago       Exited (137) About a minute ago                       tomcat

# 调用 Docker 的 API，查询容器的状态、退出码以及错误信息，然后确定容器退出的原因

# 显示容器状态，jq用来格式化json输出， -f 选项设置只输出容器的状态
$ docker inspect tomcat -f '{{json .State}}' | jq
{
  "Status": "exited",
  "Running": false,
  "Paused": false,
  "Restarting": false,
  "OOMKilled": true,
  "Dead": false,
  "Pid": 0,
  "ExitCode": 137,
  "Error": "",
  ...
}

容器已经处于 exited 状态，OOMKilled 是 true，ExitCode 是 137。OOMKilled 表示容器被 OOM 杀死了
在终端中执行 dmesg 命令，查看系统日志，并定位 OOM 相关的日志：

[193038.106468] Task in /docker/0f2b3fcdd2578165ea77266cdc7b1ad43e75877b0ac1889ecda30a78cb78bd53 killed as a result of limit of /docker/0f2b3fcdd2578165ea77266cdc7b1ad43e75877b0ac1889ecda30a78cb78bd53[193038.106478] memory: usage 524288kB, limit 524288kB, failcnt 77[193038.106480] memory+swap: usage 0kB, limit 9007199254740988kB, failcnt 0[193038.106481] kmem: usage 3708kB, limit 9007199254740988kB, failcnt 0[193038.106481] Memory cgroup stats for /docker/0f2b3fcdd2578165ea77266cdc7b1ad43e75877b0ac1889ecda30a78cb78bd53: cache:0KB rss:520580KB rss_huge:450560KB shmem:0KB mapped_file:0KB dirty:0KB writeback:0KB inactive_anon:0KB active_anon:520580KB inactive_file:0KB active_file:0KB unevictable:0KB[193038.106494] [ pid ] uid tgid total_vm rss pgtables_bytes swapents oom_score_adj name[193038.106571] [27281] 0 27281 1153302 134371 1466368 0 0 java[193038.106574] Memory cgroup out of memory: Kill process 27281 (java) score 1027 or sacrifice child[193038.1

java 进程是在容器内运行的，而容器内存的使用量和限制都是 512M（524288kB）。目前使用量已经达到了限制，所以会导致 OOM，Tomcat 容器的内存主要用在了匿名内存中，而匿名内存，其实就是主动申请分配的堆内存

# 重新启动 tomcat 容器，并调用 java 命令行来查看堆内存大小
# 重新启动容器
$ docker rm -f tomcat
$ docker run --name tomcat --cpus 0.1 -m 512M -p 8080:8080 -itd feisky/tomcat:8

# 查看堆内存，注意单位是字节
$ docker exec tomcat java -XX:+PrintFlagsFinal -version | grep HeapSize
    uintx ErgoHeapSizeLimit                         = 0                                   {product}
    uintx HeapSizePerGCThread                       = 87241520                            {product}
    uintx InitialHeapSize                          := 132120576                           {product}
    uintx LargePageHeapSizeThreshold                = 134217728                           {product}
    uintx MaxHeapSize                              := 2092957696                          {product}
    
初始堆内存的大小（InitialHeapSize）是 126MB

通过环境变量 JAVA_OPTS=’-Xmx512m -Xms512m’ ，把 JVM 的初始内存和最大内存都设为 512MB解决此问题
如果在 Docker 容器中运行 Java 应用，一定要确保，在设置容器资源限制的同时，配置好 JVM 的资源选项（比如堆内存等）。如果升级到 Java 10 ，就可以自动解决类似问题了
```

### 服务器总是时不时丢包，该怎么办？

丢包：是指在网络数据的收发过程中，由于种种原因，数据包还没传输到应用程序中，就被丢弃了。这些被丢弃包的数量，除以总的传输包数，就是丢包率。丢包率是网络性能中最核心的指标之一。丢包通常会带来严重的性能下降，特别是对 TCP 来说，丢包通常意味着网络拥塞和重传，进而还会导致网络延迟增大、吞吐降低

```
// 因为 ping 基于 ICMP 协议，而 Nginx 使用的是 TCP 协议
# -c表示发送10个请求，-S表示使用TCP SYN，-p指定端口为80
$ hping3 -c 10 -S -p 80 192.168.0.30
HPING 192.168.0.30 (eth0 192.168.0.30): S set, 40 headers + 0 data bytes
len=44 ip=192.168.0.30 ttl=63 DF id=0 sport=80 flags=SA seq=3 win=5120 rtt=7.5 ms
len=44 ip=192.168.0.30 ttl=63 DF id=0 sport=80 flags=SA seq=4 win=5120 rtt=7.4 ms
len=44 ip=192.168.0.30 ttl=63 DF id=0 sport=80 flags=SA seq=5 win=5120 rtt=3.3 ms
len=44 ip=192.168.0.30 ttl=63 DF id=0 sport=80 flags=SA seq=7 win=5120 rtt=3.0 ms
len=44 ip=192.168.0.30 ttl=63 DF id=0 sport=80 flags=SA seq=6 win=5120 rtt=3027.2 ms

--- 192.168.0.30 hping statistic ---
10 packets transmitted, 5 packets received, 50% packet loss
round-trip min/avg/max = 3.0/609.7/3027.2 ms

发送了 10 个请求包，却只收到了 5 个回复，50% 的包都丢了。再观察每个请求的 RTT 可以发现，RTT 也有非常大的波动变化，小的时候只有 3ms，而大的时候则有 3s
```

![img](https://static001.geekbang.org/resource/image/dd/fd/dd5b4050d555b1c23362456e357dfffd.png)

丢包的可能：

- 在两台 VM 连接之间，可能会发生传输失败的错误，比如网络拥塞、线路错误等
- 在网卡收包后，环形缓冲区可能会因为溢出而丢包
- 在链路层，可能会因为网络帧校验失败、QoS 等而丢包
- 在 IP 层，可能会因为路由失败、组包大小超过 MTU 等而丢包
- 在传输层，可能会因为端口未监听、资源占用超过内核限制等而丢包
- 在套接字层，可能会因为套接字缓冲区溢出而丢包
- 在应用层，可能会因为应用程序异常而丢包
- 如果配置了 iptables 规则，这些网络包也可能因为 iptables 过滤规则而丢包

#####  链路层

可以通过 ethtool 或者 netstat ，来查看网卡的丢包记录

```
root@nginx:/# netstat -i
Kernel Interface table
Iface      MTU    RX-OK RX-ERR RX-DRP RX-OVR    TX-OK TX-ERR TX-DRP TX-OVR Flg
eth0       100       31      0      0 0             8      0      0      0 BMRU
lo       65536        0      0      0 0             0      0      0      0 LRU

RX-OK、RX-ERR、RX-DRP、RX-OVR ，分别表示接收时的总包数、总错误数、进入 Ring Buffer 后因其他原因（如内存不足）导致的丢包数以及 Ring Buffer 溢出导致的丢包数，TX-OK、TX-ERR、TX-DRP、TX-OVR 也代表类似的含义，只不过是指发送时对应的各个指标

没有发现任何错误，说明容器的虚拟网卡没有丢包

检查 eth0 上是否配置了 tc 规则，并查看有没有丢包。容器终端中，执行下面的 tc 命令，添加 -s 选项，以输出统计信息，如果用 tc 等工具配置了 QoS，那么 tc 规则导致的丢包，就不会包含在网卡的统计信息中
root@nginx:/# tc -s qdisc show dev eth0
qdisc netem 800d: root refcnt 2 limit 1000 loss 30%
 Sent 432 bytes 8 pkt (dropped 4, overlimits 0 requeues 0)
 backlog 0b 0p requeues 0

eth0 上面配置了一个网络模拟排队规则（qdisc netem），并且配置了丢包率为 30%（loss 30%）。发送了 8 个包，但是丢了 4 个。应该就是这里，导致 Nginx 回复的响应包，被 netem 模块给丢了，删除 tc 中的 netem 模块
root@nginx:/# tc qdisc del dev eth0 root netem loss 30%

$ hping3 -c 10 -S -p 80 192.168.0.30
HPING 192.168.0.30 (eth0 192.168.0.30): S set, 40 headers + 0 data bytes
len=44 ip=192.168.0.30 ttl=63 DF id=0 sport=80 flags=SA seq=0 win=5120 rtt=7.9 ms
len=44 ip=192.168.0.30 ttl=63 DF id=0 sport=80 flags=SA seq=2 win=5120 rtt=1003.8 ms
len=44 ip=192.168.0.30 ttl=63 DF id=0 sport=80 flags=SA seq=5 win=5120 rtt=7.6 ms
len=44 ip=192.168.0.30 ttl=63 DF id=0 sport=80 flags=SA seq=6 win=5120 rtt=7.4 ms
len=44 ip=192.168.0.30 ttl=63 DF id=0 sport=80 flags=SA seq=9 win=5120 rtt=3.0 ms

--- 192.168.0.30 hping statistic ---
10 packets transmitted, 5 packets received, 50% packet loss
round-trip min/avg/max = 3.0/205.9/1003.8 ms

跟前面现象一样，还是 50% 的丢包；RTT 的波动也仍旧很大，从 3ms 到 1s，继续向上层分析
```

##### 网络层和传输层

在容器终端中，执行下面的 netstat -s 命令，就可以看到协议的收发汇总，以及错误信息

```
root@nginx:/# netstat -s
Ip:
    Forwarding: 1          //开启转发
    31 total packets received    //总收包数
    0 forwarded            //转发包数
    0 incoming packets discarded  //接收丢包数
    25 incoming packets delivered  //接收的数据包数
    15 requests sent out      //发出的数据包数
Icmp:
    0 ICMP messages received    //收到的ICMP包数
    0 input ICMP message failed    //收到ICMP失败数
    ICMP input histogram:
    0 ICMP messages sent      //ICMP发送数
    0 ICMP messages failed      //ICMP失败数
    ICMP output histogram:
Tcp:
    0 active connection openings  //主动连接数
    0 passive connection openings  //被动连接数
    11 failed connection attempts  //失败连接尝试数
    0 connection resets received  //接收的连接重置数
    0 connections established    //建立连接数
    25 segments received      //已接收报文数
    21 segments sent out      //已发送报文数
    4 segments retransmitted    //重传报文数
    0 bad segments received      //错误报文数
    0 resets sent          //发出的连接重置数
Udp:
    0 packets received
    ...
TcpExt:
    11 resets received for embryonic SYN_RECV sockets  //半连接重置数
    0 packet headers predicted
    TCPTimeouts: 7    //超时数
    TCPSynRetrans: 4  //SYN重传数
  ...
  
TCP 协议有多次超时和失败重试，并且主要错误是半连接重置。主要的失败，都是三次握手失败  
```

##### iptables

除了网络层和传输层的各种协议，iptables 和内核的连接跟踪机制也可能会导致丢包

```
# 容器终端中执行exit
root@nginx:/# exit
exit

# 主机终端中查询内核配置
$ sysctl net.netfilter.nf_conntrack_max
net.netfilter.nf_conntrack_max = 262144
$ sysctl net.netfilter.nf_conntrack_count
net.netfilter.nf_conntrack_count = 182

接跟踪数只有 182，而最大连接跟踪数则是 262144。这里的丢包，不可能是连接跟踪导致
```

iptables 规则，统一管理在一系列的表中，包括 filter（用于过滤）、nat（用于 NAT）、mangle（用于修改分组数据） 和 raw（用于原始数据包）等。而每张表又可以包括一系列的链，用于对 iptables 规则进行分组管理。对于丢包问题来说，最大的可能就是被 filter 表中的规则给丢弃了。需要确认，那些目标为 DROP 和 REJECT 等会弃包的规则，有没有被执行到。简单的方法，就是直接查询 DROP 和 REJECT 等规则的统计信息，看看是否为 0。如果统计值不是 0，再把相关的规则拎出来进行分析，可以通过 iptables -nvL 命令，查看各条规则的统计信息

```

# 在主机中执行
$ docker exec -it nginx bash

# 在容器中执行
root@nginx:/# iptables -t filter -nvL
Chain INPUT (policy ACCEPT 25 packets, 1000 bytes)
 pkts bytes target     prot opt in     out     source               destination
    6   240 DROP       all  --  *      *       0.0.0.0/0            0.0.0.0/0            statistic mode random probability 0.29999999981

Chain FORWARD (policy ACCEPT 0 packets, 0 bytes)
 pkts bytes target     prot opt in     out     source               destination

Chain OUTPUT (policy ACCEPT 15 packets, 660 bytes)
 pkts bytes target     prot opt in     out     source               destination
    6   264 DROP       all  --  *      *       0.0.0.0/0            0.0.0.0/0            statistic mode random probability 0.29999999981
    
两条 DROP 规则的统计数值不是 0，分别在 INPUT 和 OUTPUT 链中。这两条规则实际上是一样的，指的是使用 statistic 模块，进行随机 30% 的丢包。它们的匹配规则。0.0.0.0/0 表示匹配所有的源 IP 和目的 IP，也就是会对所有包都进行随机 30% 的丢包。这应该就是导致部分丢包的原因

// 执行下面的两条 iptables 命令，删除这两条 DROP 规则
root@nginx:/# iptables -t filter -D INPUT -m statistic --mode random --probability 0.30 -j DROP
root@nginx:/# iptables -t filter -D OUTPUT -m statistic --mode random --probability 0.30 -j DROP

$ hping3 -c 10 -S -p 80 192.168.0.30
HPING 192.168.0.30 (eth0 192.168.0.30): S set, 40 headers + 0 data bytes
len=44 ip=192.168.0.30 ttl=63 DF id=0 sport=80 flags=SA seq=0 win=5120 rtt=11.9 ms
len=44 ip=192.168.0.30 ttl=63 DF id=0 sport=80 flags=SA seq=1 win=5120 rtt=7.8 ms
...
len=44 ip=192.168.0.30 ttl=63 DF id=0 sport=80 flags=SA seq=9 win=5120 rtt=15.0 ms

--- 192.168.0.30 hping statistic ---
10 packets transmitted, 10 packets received, 0% packet loss
round-trip min/avg/max = 3.3/7.9/15.0 ms

已经没有丢包了，并且延迟的波动变化也很小。丢包问题应该已经解决了，hping3 工具，只能验证 80 端口处于正常监听状态，却还没有访问 Nginx 的 HTTP 服务

$ curl --max-time 3 http://192.168.0.30
curl: (28) Operation timed out after 3000 milliseconds with 0 bytes received

用 hping3 验证了端口正常，现在却发现 HTTP 连接超时
```

##### tcpdump

```
// 另一终端执行curl的时候此终端抓包
root@nginx:/# tcpdump -i eth0 -nn port 80
tcpdump: verbose output suppressed, use -v or -vv for full protocol decode
listening on eth0, link-type EN10MB (Ethernet), capture size 262144 bytes

// curl执行完毕后，查看 tcpdump 的输出
14:40:00.589235 IP 10.255.255.5.39058 > 172.17.0.2.80: Flags [S], seq 332257715, win 29200, options [mss 1418,sackOK,TS val 486800541 ecr 0,nop,wscale 7], length 0
14:40:00.589277 IP 172.17.0.2.80 > 10.255.255.5.39058: Flags [S.], seq 1630206251, ack 332257716, win 4880, options [mss 256,sackOK,TS val 2509376001 ecr 486800541,nop,wscale 7], length 0
14:40:00.589894 IP 10.255.255.5.39058 > 172.17.0.2.80: Flags [.], ack 1, win 229, options [nop,nop,TS val 486800541 ecr 2509376001], length 0
14:40:03.589352 IP 10.255.255.5.39058 > 172.17.0.2.80: Flags [F.], seq 76, ack 1, win 229, options [nop,nop,TS val 486803541 ecr 2509376001], length 0
14:40:03.589417 IP 172.17.0.2.80 > 10.255.255.5.39058: Flags [.], ack 1, win 40, options [nop,nop,TS val 2509379001 ecr 486800541,nop,nop,sack 1 {76:77}], length 0

前三个包是正常的 TCP 三次握手，没问题；但第四个包却是在 3 秒以后了，并且还是客户端（VM2）发送过来的 FIN 包，客户端的连接关闭了，curl 设置的 3 秒超时选项，超时后退出

// 执行 netstat -i 命令，确认一下网卡有没有丢包，接收丢包数（RX-DRP）是 344，果然是在网卡接收时丢包了
root@nginx:/# netstat -i
Kernel Interface table
Iface      MTU    RX-OK RX-ERR RX-DRP RX-OVR    TX-OK TX-ERR TX-DRP TX-OVR Flg
eth0       100      157      0    344 0            94      0      0      0 BMRU
lo       65536        0      0      0 0             0      0      0      0 LRU

第二列正是每个网卡的 MTU 值。eth0 的 MTU 只有 100，而以太网的 MTU 默认值是 1500
```

![img](https://static001.geekbang.org/resource/image/a8/c2/a81bd7639a1f81c23bc6d2e030af97c2.png)

hping3：实际上只发送了 SYN 包

curl：在发送 SYN 包后，还会发送 HTTP GET 请求。HTTP GET ，本质上也是一个 TCP 包，但跟 SYN 包相比，它还携带了 HTTP GET 的数据

```
// 把容器 eth0 的 MTU 改成 1500，再次执行 curl 命令，确认问题真的解决了
root@nginx:/# ifconfig eth0 mtu 1500
```

### 内核线程 CPU 利用率太高，该怎么办？

##### 内核线程

在 Linux 中，用户态进程的“祖先”，都是 PID 号为 1 的 init 进程。现在主流的 Linux 发行版中，init 都是 systemd 进程；而其他的用户态进程，会通过 systemd 来进行管理。Linux 在启动过程中，有三个特殊的进程，也就是 PID 号最小的三个进程：

* 0 号进程为 idle 进程，系统创建的第一个进程，它在初始化 1 号和 2 号进程后，演变为空闲任务。当 CPU 上没有其他任务执行时，就会运行它
* 1 号进程为 init 进程，通常是 systemd 进程，在用户态运行，用来管理其他用户态进程
* 2 号进程为 kthreadd 进程，在内核态运行，用来管理内核线程

```
// 要查找内核线程，从 2 号进程开始，查找它的子孙进程即可。使用 ps 命令，来查找 kthreadd 的子进程
$ ps -f --ppid 2 -p 2
UID         PID   PPID  C STIME TTY          TIME CMD
root          2      0  0 12:02 ?        00:00:01 [kthreadd]
root          9      2  0 12:02 ?        00:00:21 [ksoftirqd/0]
root         10      2  0 12:02 ?        00:11:47 [rcu_sched]
root         11      2  0 12:02 ?        00:00:18 [migration/0]
...
root      11094      2  0 14:20 ?        00:00:00 [kworker/1:0-eve]
root      11647      2  0 14:27 ?        00:00:00 [kworker/0:2-cgr]

内核线程的名称（CMD）都在中括号里，更简单的方法，就是直接查找名称包含中括号的进程
$ ps -ef | grep "\[.*\]"
root         2     0  0 08:14 ?        00:00:00 [kthreadd]
root         3     2  0 08:14 ?        00:00:00 [rcu_gp]
root         4     2  0 08:14 ?        00:00:00 [rcu_par_gp]
...
```

除了 kthreadd 和 ksoftirqd 外，还有很多常见的内核线程，在性能分析中都经常会碰到，比如

* kswapd0：用于内存回收
* kworker：用于执行内核工作队列，分为绑定 CPU （名称格式为 kworker/CPU86330）和未绑定 CPU（名称格式为 kworker/uPOOL86330）两类
* migration：在负载均衡过程中，把进程迁移到 CPU 上。每个 CPU 都有一个 migration 内核线程
* jbd2/sda1-8：jbd 是 Journaling Block Device 的缩写，用来为文件系统提供日志功能，以保证数据的完整性；名称中的 sda1-8，表示磁盘分区名称和设备号。每个使用了 ext4 文件系统的磁盘分区，都会有一个 jbd2 内核线程
* pdflush：用于将内存中的脏页（被修改过，但还未写入磁盘的文件页）写入磁盘（已经在 3.10 中合并入了 kworker 中）

```
// 第二个终端
$ curl http://192.168.0.30/
// 第二个终端
# -S参数表示设置TCP协议的SYN（同步序列号），-p表示目的端口为80
# -i u10表示每隔10微秒发送一个网络帧
# 注：如果你在实践过程中现象不明显，可以尝试把10调小，比如调成5甚至1
$ hping3 -S -p 80 -i u10 192.168.0.30

// 回到第一个终端，就会发现异常——系统的响应明显变慢了。执行 top，观察一下系统和进程的 CPU 使用情况
$ top
top - 08:31:43 up 17 min,  1 user,  load average: 0.00, 0.00, 0.02
Tasks: 128 total,   1 running,  69 sleeping,   0 stopped,   0 zombie
%Cpu0  :  0.3 us,  0.3 sy,  0.0 ni, 66.8 id,  0.3 wa,  0.0 hi, 32.4 si,  0.0 st
%Cpu1  :  0.0 us,  0.3 sy,  0.0 ni, 65.2 id,  0.0 wa,  0.0 hi, 34.5 si,  0.0 st
KiB Mem :  8167040 total,  7234236 free,   358976 used,   573828 buff/cache
KiB Swap:        0 total,        0 free,        0 used.  7560460 avail Mem

  PID USER      PR  NI    VIRT    RES    SHR S  %CPU %MEM     TIME+ COMMAND
    9 root      20   0       0      0      0 S   7.0  0.0   0:00.48 ksoftirqd/0
   18 root      20   0       0      0      0 S   6.9  0.0   0:00.56 ksoftirqd/1
 2489 root      20   0  876896  38408  21520 S   0.3  0.5   0:01.50 docker-containe
 3008 root      20   0   44536   3936   3304 R   0.3  0.0   0:00.09 top
    1 root      20   0   78116   9000   6432 S   0.0  0.1   0:11.77 systemd
 ...
 
两个 CPU 的软中断使用率都超过了 30%；CPU 使用率最高的进程，是软中断内核线程 ksoftirqd/0 和 ksoftirqd/1 
```

对于普通进程，观察其行为有很多方法，比如 strace、pstack、lsof 等等。但这些工具并不适合内核线程，比如，如果用 pstack ，或者通过 /proc/pid/stack 查看 ksoftirqd/0（进程号为 9）的调用栈时，分别可以得到以下输出

```
$ pstack 9
Could not attach to target 9: Operation not permitted.
detach: No such process

$ cat /proc/9/stack
[<0>] smpboot_thread_fn+0x166/0x170
[<0>] kthread+0x121/0x140
[<0>] ret_from_fork+0x35/0x40
[<0>] 0xffffffffffffffff

pstack 报出的是不允许挂载进程的错误；而 /proc/9/stack 方式虽然有输出，但输出中并没有详细的调用栈情况

// 终端一中执行下面的 perf record 命令；并指定进程号 9 ，以便记录 ksoftirqd 的行为
# 采样30s后退出
$ perf record -a -g -p 9 -- sleep 30

继续执行 perf report命令，得到 perf 的汇总报告。按上下方向键以及回车键，展开比例最高的 ksoftirqd 后，可以得到调用关系链图，更好的方法，来查看整个调用栈的信息的方式是火焰图
```

##### 火焰图

针对 perf 汇总数据的展示问题，Brendan Gragg 发明了火焰图，通过矢量图的形式，更直观展示汇总结果

* 横轴表示采样数和采样比例。一个函数占用的横轴越宽，就代表它的执行时间越长。同一层的多个函数，则是按照字母来排序
* 纵轴表示调用栈，由下往上根据调用关系逐个展开。上下相邻的两个函数中，下面的函数，是上面函数的父函数。调用栈越深，纵轴就越高

火焰图可以分为下面这几种

* on-CPU 火焰图：表示 CPU 的繁忙情况，用在 CPU 使用率比较高的场景中
* off-CPU 火焰图：表示 CPU 等待 I/O、锁等各种资源的阻塞情况
* 内存火焰图：表示内存的分配和释放情况
* 热 / 冷火焰图：表示将 on-CPU 和 off-CPU 结合在一起综合展示
* 差分火焰图：表示两个火焰图的差分情况，红色表示增长，蓝色表示衰减。差分火焰图常用来比较不同场景和不同时期的火焰图，以便分析系统变化前后对性能的影响情况

```
// 安装火焰图工具
$ git clone https://github.com/brendangregg/FlameGraph
$ cd FlameGraph
```

生成火焰图，其实主要需要三个步骤：

1. 执行 perf script ，将 perf record 的记录转换成可读的采样记录
2. 执行 stackcollapse-perf.pl 脚本，合并调用栈信息
3. 执行 flamegraph.pl 脚本，生成火焰图

或者利用管道简化：

```
// 假设刚才用 perf record 生成的文件路径为 /root/perf.data
$ perf script -i /root/perf.data | ./stackcollapse-perf.pl --all |  ./flamegraph.pl > ksoftirqd.svg
```

使用浏览器打开 ksoftirqd.svg ，你就可以看到生成的火焰图了

![img](https://static001.geekbang.org/resource/image/6d/cd/6d4f1fece12407906aacedf5078e53cd.png)

顺着调用栈由下往上看（顺着图中蓝色箭头），就可以得到跟刚才 perf report 中一样的结果，到了 ip_forward 这里，已经看不清函数名称了。需要点击 ip_forward，展开最上面这一块调用栈

![img](https://static001.geekbang.org/resource/image/41/a3/416291ba2f9c039a0507f913572a21a3.png)

进一步看到 ip_forward 后的行为，也就是把网络包发送出去。根据这个调用过程，再结合网络收发和 TCP/IP 协议栈原理，这个流程中的网络接收、网桥以及 netfilter 调用等，都是导致软中断 CPU 升高的重要因素，也就是影响网络性能的潜在瓶颈。

火焰图中的颜色，并没有特殊含义，只是用来区分不同的函数，整个火焰图不包含任何时间的因素，所以并不能看出横向各个函数的执行次序

实际上，火焰图方法同样适用于普通进程

### 动态追踪怎么用？

##### 动态追踪

动态追踪技术，通过探针机制，来采集内核或者应用程序的运行信息，从而可以不用修改内核和应用程序的代码，就获得丰富的信息，帮你分析、定位想要排查的问题。相比以往的进程级跟踪方法（比如 ptrace），动态追踪往往只会带来很小的性能损耗（通常在 5% 或者更少）

* DTrace 是动态追踪技术的鼻祖，它提供了一个通用的观测框架，可以跟踪用户态和内核态的所有事件，并通过一些列的优化措施，保证最小的性能开销。虽然直到今天，DTrace 本身依然无法在 Linux 中运行，但它同样对 Linux 动态追踪产生了巨大的影响
* SystemTap 也定义了一种类似的脚本语言，方便用户根据需要自由扩展。不过，不同于 DTrace，SystemTap 并没有常驻内核的运行时，它需要先把脚本编译为内核模块，然后再插入到内核中执行。这也导致 SystemTap 启动比较缓慢，并且依赖于完整的调试符号表

为了追踪内核或用户空间的事件，Dtrace 和 SystemTap 都会把用户传入的追踪处理函数（一般称为 Action），关联到被称为探针的检测点上。这些探针，实际上也就是各种动态追踪技术所依赖的事件源

#####  动态追踪的事件源

根据事件类型的不同，动态追踪所使用的事件源，可以分为静态探针、动态探针以及硬件事件等三类。它们的关系如下图所示

![img](https://static001.geekbang.org/resource/image/ba/61/ba6c9ed0dcccc7f4f46bb19c69946e61.png)

* 硬件事件通常由性能监控计数器 PMC（Performance Monitoring Counter）产生，包括了各种硬件的性能情况，比如 CPU 的缓存、指令周期、分支预测等等
* 静态探针，是指事先在代码中定义好，并编译到应用程序或者内核中的探针。这些探针只有在开启探测功能时，才会被执行到；未开启时并不会执行。常见的静态探针包括：
  * 跟踪点（tracepoints），实际上就是在源码中插入的一些带有控制条件的探测点，这些探测点允许事后再添加处理函数。比如在内核中，最常见的静态跟踪方法就是 printk，即输出日志。Linux 内核定义了大量的跟踪点，可以通过内核编译选项，来开启或者关闭
  * USDT 探针，全称是用户级静态定义跟踪，需要在源码中插入 DTRACE_PROBE() 代码，并编译到应用程序中。不过，也有很多应用程序内置了 USDT 探针，比如 MySQL、PostgreSQL 等

* 动态探针，则是指没有事先在代码中定义，但却可以在运行时动态添加的探针，比如函数的调用和返回等。动态探针支持按需在内核或者应用程序中添加探测点，具有更高的灵活性。常见的动态探针有两种：
  * kprobes 用来跟踪内核态的函数，包括用于函数调用的 kprobe 和用于函数返回的 kretprobe，需要内核编译时开启 CONFIG_KPROBE_EVENTS
  * uprobes 用来跟踪用户态的函数，包括用于函数调用的 uprobe 和用于函数返回的 uretprobe，需要内核编译时开启 CONFIG_UPROBE_EVENTS

##### 动态追踪机制

* ftrace 最早用于函数跟踪，后来又扩展支持了各种事件跟踪功能。ftrace 的使用接口跟 procfs 类似，它通过 debugfs（4.1 以后也支持 tracefs），以普通文件的形式，向用户空间提供访问接口。这样，不需要额外的工具，就可以通过挂载点（通常为 /sys/kernel/debug/tracing 目录）内的文件读写，来跟 ftrace 交互，跟踪内核或者应用程序的运行事件
* perf 提供了事件记录和分析功能，这实际上只是一种最简单的静态跟踪机制。也可以通过 perf ，来自定义动态事件（perf probe），只关注真正感兴趣的事件
* eBPF 则在 BPF（Berkeley Packet Filter）的基础上扩展而来，不仅支持事件跟踪机制，还可以通过自定义的 BPF 代码（使用 C 语言）自由扩展。eBPF 实际上就是常驻于内核的运行时，可以说就是 Linux 版的 DTrace
* 除此之外，还有很多内核外的工具，也提供了丰富的动态追踪功能。最常见的就是前面提到的 SystemTap， BCC（BPF Compiler Collection），以及常用于容器性能分析的 sysdig 等

##### ftrace

```
$ cd /sys/kernel/debug/tracing

// 如果这个目录不存在，则说明系统还没有挂载 debugfs，可以执行下面的命令来挂载它
$ mount -t debugfs nodev /sys/kernel/debug
```

ftrace 提供了多个跟踪器，用于跟踪不同类型的信息，比如函数调用、中断关闭、进程调度等。具体支持的跟踪器取决于系统配置，可以执行下面的命令，来查询所有支持的跟踪器：

```
$ cat available_tracers
hwlat blk mmiotrace function_graph wakeup_dl wakeup_rt wakeup function nop
```

使用 ftrace 前，还需要确认跟踪目标，包括内核函数和内核事件。其中，函数就是内核中的函数名。而事件，则是内核源码中预先定义的跟踪点，可以执行下面的命令，来查询支持的函数和事件：

```
$ cat available_filter_functions
$ cat available_events
```

以 ls 命令为例，一起看看 ftrace 的使用方法

```
// ls 命令会通过 open 系统调用打开目录文件，而 open 在内核中对应的函数名为 do_sys_open。 要跟踪的函数设置为 do_sys_open
$ echo do_sys_open > set_graph_function

// 配置跟踪选项，开启函数调用跟踪，并跟踪调用进程
$ echo function_graph > current_tracer
$ echo funcgraph-proc > trace_options

// 开启跟踪
$ echo 1 > tracing_on

// 执行一个 ls 命令后，再关闭跟踪
$ ls
$ echo 0 > tracing_on

// 最后一步，查看跟踪结果
$ cat trace
# tracer: function_graph
#
# CPU  TASK/PID         DURATION                  FUNCTION CALLS
# |     |    |           |   |                     |   |   |   |
 0)    ls-12276    |               |  do_sys_open() {
 0)    ls-12276    |               |    getname() {
 0)    ls-12276    |               |      getname_flags() {
 0)    ls-12276    |               |        kmem_cache_alloc() {
 0)    ls-12276    |               |          _cond_resched() {
 0)    ls-12276    |   0.049 us    |            rcu_all_qs();
 0)    ls-12276    |   0.791 us    |          }
 0)    ls-12276    |   0.041 us    |          should_failslab();
 0)    ls-12276    |   0.040 us    |          prefetch_freepointer();
 0)    ls-12276    |   0.039 us    |          memcg_kmem_put_cache();
 0)    ls-12276    |   2.895 us    |        }
 0)    ls-12276    |               |        __check_object_size() {
 0)    ls-12276    |   0.067 us    |          __virt_addr_valid();
 0)    ls-12276    |   0.044 us    |          __check_heap_object();
 0)    ls-12276    |   0.039 us    |          check_stack_object();
 0)    ls-12276    |   1.570 us    |        }
 0)    ls-12276    |   5.790 us    |      }
 0)    ls-12276    |   6.325 us    |    }
...

第一列表示运行的 CPU；第二列是任务名称和进程 PID；第三列是函数执行延迟；最后一列，则是函数调用关系图

trace-cmd 已经把这些步骤给包装了起来。可以在同一个命令行工具里，完成上述所有过程
$ trace-cmd record -p function_graph -g do_sys_open -O funcgraph-proc ls
$ trace-cmd report
```

##### perf

1. 内核函数 do_sys_open

通过 perf list ，查询所有支持的事件

```
$ perf list
```

在 perf 的各个子命令中添加 --event 选项，设置追踪感兴趣的事件。如果这些预定义的事件不满足实际需要，还可以使用 perf probe 来动态添加。除了追踪内核事件外，perf 还可以用来跟踪用户空间的函数

```
// 可以执行 perf probe 命令，添加 do_sys_open 探针
$ perf probe --add do_sys_open
Added new event:
  probe:do_sys_open    (on do_sys_open)
You can now use it in all perf tools, such as:
    perf record -e probe:do_sys_open -aR sleep 1
    
// 可以对 10s 内的 do_sys_open 进行采样
$ perf record -e probe:do_sys_open -aR sleep 10
[ perf record: Woken up 1 times to write data ]
[ perf record: Captured and wrote 0.148 MB perf.data (19 samples) ]

// 采样成功后，就可以执行 perf script ，来查看采样结果
$ perf script
            perf 12886 [000] 89565.879875: probe:do_sys_open: (ffffffffa807b290)
           sleep 12889 [000] 89565.880362: probe:do_sys_open: (ffffffffa807b290)
           sleep 12889 [000] 89565.880382: probe:do_sys_open: (ffffffffa807b290)
           sleep 12889 [000] 89565.880635: probe:do_sys_open: (ffffffffa807b290)
           sleep 12889 [000] 89565.880669: probe:do_sys_open: (ffffffffa807b290)
           
// 执行下面的命令，你就可以知道 do_sys_open 的所有参数  
$ perf probe -V do_sys_open
Available variables at do_sys_open
        @<do_sys_open+0>
                char*   filename
                int     dfd
                int     flags
                struct open_flags       op
                umode_t mode
                
// 参数名称为 filename。如果这个命令执行失败，就说明调试符号表还没有安装。执行下面的命令，安装调试信息后重试 
$ apt-get install linux-image-`uname -r`-dbgsym          

# 先删除旧的探针
perf probe --del probe:do_sys_open

# 添加带参数的探针
$ perf probe --add 'do_sys_open filename:string'
Added new event:
  probe:do_sys_open    (on do_sys_open with filename:string)
You can now use it in all perf tools, such as:
    perf record -e probe:do_sys_open -aR sleep 1
    
// 重新执行 record 和 script 子命令，采样并查看记录
# 重新采样记录
$ perf record -e probe:do_sys_open -aR ls

# 查看结果
$ perf script
            perf 13593 [000] 91846.053622: probe:do_sys_open: (ffffffffa807b290) filename_string="/proc/13596/status"
              ls 13596 [000] 91846.053995: probe:do_sys_open: (ffffffffa807b290) filename_string="/etc/ld.so.cache"
              ls 13596 [000] 91846.054011: probe:do_sys_open: (ffffffffa807b290) filename_string="/lib/x86_64-linux-gnu/libselinux.so.1"
              ls 13596 [000] 91846.054066: probe:do_sys_open: (ffffffffa807b290) filename_string="/lib/x86_64-linux-gnu/libc.so.6”
              ...
# 使用完成后不要忘记删除探针
$ perf probe --del probe:do_sys_open    
```

strace 也能得到类似结果，本身又容易操作，从原理上来说，strace 基于系统调用 ptrace 实现，这就带来了两个问题：

* ptrace 是系统调用，要在内核态和用户态切换。当事件数量比较多时，繁忙的切换必然会影响原有服务性能
* ptrace 需要借助 SIGSTOP 信号挂起目标进程。这种信号控制和进程挂起，会影响目标进程的行为

```
$ strace ls
...
access("/etc/ld.so.nohwcap", F_OK)      = -1 ENOENT (No such file or directory)
access("/etc/ld.so.preload", R_OK)      = -1 ENOENT (No such file or directory)
openat(AT_FDCWD, "/etc/ld.so.cache", O_RDONLY|O_CLOEXEC) = 3
...
access("/etc/ld.so.nohwcap", F_OK)      = -1 ENOENT (No such file or directory)
openat(AT_FDCWD, "/lib/x86_64-linux-gnu/libselinux.so.1", O_RDONLY|O_CLOEXEC) = 3
...
```

在 strace 的启发下，结合内核中的 utrace 机制， perf 也提供了一个 trace 子命令，是取代 strace 的首选工具。相对于 ptrace 机制来说，perf trace 基于内核事件，自然要比进程跟踪的性能好很多

```
$ perf trace ls
         ? (         ): ls/14234  ... [continued]: execve()) = 0
     0.177 ( 0.013 ms): ls/14234 brk(                                                                  ) = 0x555d96be7000
     0.224 ( 0.014 ms): ls/14234 access(filename: 0xad98082                                            ) = -1 ENOENT No such file or directory
     0.248 ( 0.009 ms): ls/14234 access(filename: 0xad9add0, mode: R                                   ) = -1 ENOENT No such file or directory
     0.267 ( 0.012 ms): ls/14234 openat(dfd: CWD, filename: 0xad98428, flags: CLOEXEC                  ) = 3
     0.288 ( 0.009 ms): ls/14234 fstat(fd: 3</usr/lib/locale/C.UTF-8/LC_NAME>, statbuf: 0x7ffd2015f230 ) = 0
     0.305 ( 0.011 ms): ls/14234 mmap(len: 45560, prot: READ, flags: PRIVATE, fd: 3                    ) = 0x7efe0af92000
     0.324 Dockerfile  test.sh
( 0.008 ms): ls/14234 close(fd: 3</usr/lib/locale/C.UTF-8/LC_NAME>                          ) = 0
     ...
```

2. 用户空间的库函数readline

通过 -x 指定 bash 二进制文件的路径，就可以动态跟踪库函数。这其实就是跟踪了所有用户在 bash 中执行的命令

```
# 为/bin/bash添加readline探针
$ perf probe -x /bin/bash 'readline%return +0($retval):string’

# 采样记录
$ perf record -e probe_bash:readline__return -aR sleep 5

# 查看结果
$ perf script
    bash 13348 [000] 93939.142576: probe_bash:readline__return: (5626ffac1610 <- 5626ffa46739) arg1="ls"

# 跟踪完成后删除探针
$ perf probe --del probe_bash:readline__return
```

如果不确定探针格式，也可以通过下面的命令，查询所有支持的函数和函数参数

```
# 查询所有的函数
$ perf probe -x /bin/bash —funcs

# 查询函数的参数
$ perf probe -x /bin/bash -V readline
Available variables at readline
        @<readline+0>
                char*   prompt
```

##### eBPF 和 BCC 

ftrace 和 perf 的功能已经比较丰富了，不过，它们有一个共同的缺陷，那就是不够灵活，没法像 DTrace 那样通过脚本自由扩展。而 eBPF 就是 Linux 版的 DTrace，可以通过 C 语言自由扩展（这些扩展通过 LLVM 转换为 BPF 字节码后，加载到内核中执行）。从使用上来说，eBPF 要比 ftrace 和 perf ，都更加繁杂。在 eBPF 执行过程中，编译、加载还有 maps 等操作，对所有的跟踪程序来说都是通用的。把这些过程通过 Python 抽象起来，也就诞生了 BCC（BPF Compiler Collection）。BCC 把 eBPF 中的各种事件源（比如 kprobe、uprobe、tracepoint 等）和数据操作（称为 Maps），也都转换成了 Python 接口（也支持 lua）。使用 BCC 进行动态追踪时，编写简单的脚本就可以了

BBC支持的工具：

![img](https://static001.geekbang.org/resource/image/fc/21/fc5f387a982db98c49c7cefb77342c21.png)

BBC支持的特性：

![img](https://static001.geekbang.org/resource/image/61/e8/61abce1affc770a15dae7d489e50a8e8.png)

##### SystemTap 和 sysdig

* SystemTap：可以通过脚本进行自由扩展的动态追踪技术。在 eBPF 出现之前，SystemTap 是 Linux 系统中，功能最接近 DTrace 的动态追踪机制。不过SystemTap 在很长时间以来都游离于内核之外（而 eBPF 自诞生以来，一直根植在内核中）
* sysdig：随着容器技术的普及而诞生的，主要用于容器的动态追踪。sysdig 汇集了一些列性能工具的优势，可以说是集百家之所长。sysdig 的特点： sysdig = strace + tcpdump + htop + iftop + lsof + docker inspect

##### 如何选择追踪工具

* 在不需要很高灵活性的场景中，使用 perf 对性能事件进行采样，然后再配合火焰图辅助分析，就是最常用的一种方法
* 需要对事件或函数调用进行统计分析（比如观察不同大小的 I/O 分布）时，就要用 SystemTap 或者 eBPF，通过一些自定义的脚本来进行数据处理

![img](https://static001.geekbang.org/resource/image/5a/25/5a2b2550547d5eaee850bfb806f76625.png)

### 服务吞吐量下降很厉害，怎么分析？

```
# 默认测试时间为10s，请求超时2s
$ wrk --latency -c 1000 http://192.168.0.30
Running 10s test @ http://192.168.0.30
  2 threads and 1000 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency    14.82ms   42.47ms 874.96ms   98.43%
    Req/Sec   550.55      1.36k    5.70k    93.10%
  Latency Distribution
     50%   11.03ms
     75%   15.90ms
     90%   23.65ms
     99%  215.03ms
  1910 requests in 10.10s, 573.56KB read
  Non-2xx or 3xx responses: 1910
Requests/sec:    189.10
Transfer/sec:     56.78KB

吞吐量（也就是每秒请求数）只有 189，并且所有 1910 个请求收到的都是异常响应（非 2xx 或 3xx）。这些数据显然表明，吞吐量太低了，并且请求处理都失败了
```

##### 连接数优化

查看 TCP 连接数的汇总情况，首选工具自然是 ss 命令

```
# 测试时间30分钟
$ wrk --latency -c 1000 -d 1800 http://192.168.0.30
```

终端一中，观察 TCP 连接数

```
$ ss -s
Total: 177 (kernel 1565)
TCP:   1193 (estab 5, closed 1178, orphaned 0, synrecv 0, timewait 1178/0), ports 0

Transport Total     IP        IPv6
*    1565      -         -
RAW    1         0         1
UDP    2         2         0
TCP    15        12        3
INET    18        14        4
FRAG    0         0         0

wrk 并发 1000 请求时，建立连接数只有 5，而 closed 和 timewait 状态的连接则有 1100 多 。两个问题：一个是建立连接数太少了；另一个是 timewait 状态连接太多了
```

查看系统日志

```
$ dmesg | tail
[88356.354329] nf_conntrack: nf_conntrack: table full, dropping packet
[88356.354374] nf_conntrack: nf_conntrack: table full, dropping packet

连接跟踪导致的问题
```

连接跟踪数的最大限制 nf_conntrack_max ，以及当前的连接跟踪数 nf_conntrack_count

```
$ sysctl net.netfilter.nf_conntrack_max
net.netfilter.nf_conntrack_max = 200
$ sysctl net.netfilter.nf_conntrack_count
net.netfilter.nf_conntrack_count = 200

# 将连接跟踪限制增大到1048576
$ sysctl -w net.netfilter.nf_conntrack_max=1048576
```

重新测试 Nginx 的性能

```
# 默认测试时间为10s，请求超时2s
$ wrk --latency -c 1000 http://192.168.0.30
...
  54221 requests in 10.07s, 15.16MB read
  Socket errors: connect 0, read 7, write 0, timeout 110
  Non-2xx or 3xx responses: 45577
Requests/sec:   5382.21
Transfer/sec:      1.50MB

吞吐量已经从刚才的 189 增大到了 5382。看起来性能提升了将近 30 倍，不过，10s 内的总请求数虽然增大到了 5 万，但是有 4 万多响应异常，真正成功的只有 8000 多个（54221-45577=8644）
```

##### 工作进程优化

响应的状态码并不是我们期望的 2xx （表示成功）或 3xx（表示重定向），max_children 表示 php-fpm 子进程的最大数量

##### 套接字优化

观察有没有发生套接字的丢包现象

```
# 终端二，测试时间30分钟
$ wrk --latency -c 1000 -d 1800 http://192.168.0.30

# 只关注套接字统计
$ netstat -s | grep socket
    73 resets received for embryonic SYN_RECV sockets
    308582 TCP sockets finished time wait in fast timer
    8 delayed acks further delayed because of locked socket
    290566 times the listen queue of a socket overflowed
    290566 SYNs to LISTEN sockets dropped

# 稍等一会，再次运行
$ netstat -s | grep socket
    73 resets received for embryonic SYN_RECV sockets
    314722 TCP sockets finished time wait in fast timer
    8 delayed acks further delayed because of locked socket
    344440 times the listen queue of a socket overflowed
    344440 SYNs to LISTEN sockets dropped
    
丢包都是套接字队列溢出导致    
```

查看套接字的队列大小

```
$ ss -ltnp
State     Recv-Q     Send-Q            Local Address:Port            Peer Address:Port
LISTEN    10         10                      0.0.0.0:80                   0.0.0.0:*         users:(("nginx",pid=10491,fd=6),("nginx",pid=10490,fd=6),("nginx",pid=10487,fd=6))
LISTEN    7          10                            *:9000                       *:*         users:(("php-fpm",pid=11084,fd=9),...,("php-fpm",pid=10529,fd=7))

Nginx 和 php-fpm 的监听队列 （Send-Q）只有 10，而 nginx 的当前监听队列长度 （Recv-Q）已经达到了最大值，php-fpm 也已经接近了最大值，除了Nginx 和 somaxconn 的配置都是 10，php-fpm 的配置是511

# somaxconn是系统级套接字监听队列上限
$ sysctl net.core.somaxconn
net.core.somaxconn = 10
```

终端二中，重新测试 Nginx 的性能

```
$ wrk --latency -c 1000 http://192.168.0.30
...
  62247 requests in 10.06s, 18.25MB read
  Non-2xx or 3xx responses: 62247
Requests/sec:   6185.65
Transfer/sec:      1.81MB

吞吐量已经增大到了 6185，在测试的时候，如果在终端一中重新执行 netstat -s | grep socket，已经没有套接字丢包问题了。不过，这次 Nginx 的响应，再一次全部失败了，都是 Non-2xx or 3xx。再去终端一中，查看 Nginx 日志

$ docker logs nginx --tail 10
2019/03/15 16:52:39 [crit] 15#15: *999779 connect() to 127.0.0.1:9000 failed (99: Cannot assign requested address) while connecting to upstream, client: 192.168.0.2, server: localhost, request: "GET / HTTP/1.1", upstream: "fastcgi://127.0.0.1:9000", host: "192.168.0.30"

Cannot assign requested address。错误代码为 EADDRNOTAVAIL，表示 IP 地址或者端口号不可用。显然只能是端口号的问题
```

##### 端口号优化

查询系统配置的临时端口号范围

```
$ sysctl net.ipv4.ip_local_port_range
net.ipv4.ip_local_port_range=20000 20050

临时端口的范围只有 50 个，将其增大
$ sysctl -w net.ipv4.ip_local_port_range="10000 65535"
net.ipv4.ip_local_port_range = 10000 65535

$ wrk --latency -c 1000 http://192.168.0.30/
...
  32308 requests in 10.07s, 6.71MB read
  Socket errors: connect 0, read 2027, write 0, timeout 433
  Non-2xx or 3xx responses: 30
Requests/sec:   3208.58
Transfer/sec:    682.15KB

异常的响应少多了 ，不过，吞吐量也下降到了 3208。并且，这次还出现了很多 Socket read errors
```

##### 火焰图

优化了很多配置。这些配置在优化网络的同时，却也会带来其他资源使用的上升。是不是说明其他资源遇到瓶颈了呢？

```
# 终端二中测试时间30分钟
$ wrk --latency -c 1000 -d 1800 http://192.168.0.30

# 终端一中，执行 top ，观察 CPU 和内存的使用
$ top
...
%Cpu0  : 30.7 us, 48.7 sy,  0.0 ni,  2.3 id,  0.0 wa,  0.0 hi, 18.3 si,  0.0 st
%Cpu1  : 28.2 us, 46.5 sy,  0.0 ni,  2.0 id,  0.0 wa,  0.0 hi, 23.3 si,  0.0 st
KiB Mem :  8167020 total,  5867788 free,   490400 used,  1808832 buff/cache
KiB Swap:        0 total,        0 free,        0 used.  7361172 avail Mem

  PID USER      PR  NI    VIRT    RES    SHR S  %CPU %MEM     TIME+ COMMAND
20379 systemd+  20   0   38068   8692   2392 R  36.1  0.1   0:28.86 nginx
20381 systemd+  20   0   38024   8700   2392 S  33.8  0.1   0:29.29 nginx
 1558 root      20   0 1118172  85868  39044 S  32.8  1.1  22:55.79 dockerd
20313 root      20   0   11024   5968   3956 S  27.2  0.1   0:22.78 docker-containe
13730 root      20   0       0      0      0 I   4.0  0.0   0:10.07 kworker/u4:0-ev

系统 CPU 使用率（sy）比较高，两个 CPU 的系统 CPU 使用率都接近 50%，且空闲 CPU 使用率只有 2%。CPU 主要被两个 Nginx 进程和两个 docker 相关的进程占用，使用率都是 30% 左右


# 终端二中的 wrk 继续运行, 在终端一中，执行 perf 和 flamegraph 脚本，生成火焰图：
# 执行perf记录事件
$ perf record -g

# 切换到FlameGraph安装路径执行下面的命令生成火焰图
$ perf script -i ~/perf.data | ./stackcollapse-perf.pl --all | ./flamegraph.pl > nginx.svg
```

![img](https://static001.geekbang.org/resource/image/89/c6/8933557b5eb8c8f41a629e751fd7f0c6.png)

do_syscall_64、tcp_v4_connect、inet_hash_connect 这个堆栈，很明显就是最需要关注的地方。inet_hash_connect() 是 Linux 内核中负责分配临时端口号的函数。瓶颈应该还在临时端口的分配上

终端一中运行 ss 命令， 查看连接状态统计

```
$ ss -s
TCP:   32775 (estab 1, closed 32768, orphaned 0, synrecv 0, timewait 32768/0), ports 0
...

大量连接（这儿是 32768）处于 timewait 状态，而 timewait 状态的连接，本身会继续占用端口号。如果这些端口号可以重用，那么自然就可以缩短 __init_check_established 的过程。而 Linux 内核中，tcp_tw_reuse 选项，用来控制端口号的重用

$ sysctl net.ipv4.tcp_tw_reuse
net.ipv4.tcp_tw_reuse = 0
```

将tcp_tw_reuse改为1后切换到终端二中，再次测试优化后的效果

```
$ wrk --latency -c 1000 http://192.168.0.30/
...
  52119 requests in 10.06s, 10.81MB read
  Socket errors: connect 0, read 850, write 0, timeout 0
Requests/sec:   5180.48
Transfer/sec:      1.07MB

吞吐量已经达到了 5000 多，只有少量的 Socket errors，也不再有 Non-2xx or 3xx 的响应。说明一切终于正常了
```

### 系统监控的综合思路

##### USE法

* 使用率：表示资源用于服务的时间或容量百分比。100% 的使用率，表示容量已经用尽或者全部时间都用于服务
* 饱和度：表示资源的繁忙程度，通常与等待队列的长度相关。100% 的饱和度，表示资源无法接受更多的请求
* 错误数：表示发生错误的事件个数。错误数越多，表明系统的问题越严重

这三个类别的指标，涵盖了系统资源的常见性能瓶颈，所以常被用来快速定位系统资源的性能瓶颈。无论是对 CPU、内存、磁盘和文件系统、网络等硬件资源，还是对文件描述符数、连接数、连接跟踪数等软件资源，USE 方法都可以快速定位出，是哪一种系统资源出现了性能瓶颈。除了USE，还有更偏重于应用的RED法

![img](https://static001.geekbang.org/resource/image/cc/ee/ccd7a9350c270c0168bad6cc8d0b8aee.png)

##### 监控系统

一个完整的监控系统通常由数据采集、数据存储、数据查询和处理、告警以及可视化展示等多个模块组成。要从头搭建一个监控系统，其实也是一个很大的系统工程。现在已经有很多开源的监控工具可以直接使用，比如最常见的 Zabbix、Nagios、Prometheus 等等，Prometheus 的基本架构：

![img](https://static001.geekbang.org/resource/image/7f/56/7f9c36db17785097ef9d186fd782ce56.png)

1. 数据采集模块：最左边的 Prometheus targets 就是数据采集的对象，Retrieval 则负责采集这些数据。Prometheus 同时支持两种数据采集模式
   * Pull 模式：由服务器端的采集模块来触发采集。只要采集目标提供了 HTTP 接口，就可以自由接入（这也是最常用的采集模式）
   * Push 模式：由各个采集目标主动向 Push Gateway（用于防止数据丢失）推送指标，再由服务器端从 Gateway 中拉取过去（这是移动应用中最常用的采集模式）

2. 数据存储模块：为了保持监控数据的持久化，TSDB（Time series database）模块，负责将采集到的数据持久化到 SSD 等磁盘设备中。TSDB 是专门为时间序列数据设计的一种数据库，特点是以时间为索引、数据量大并且以追加的方式写入
3. 数据查询和处理模块： TSDB，在存储数据的同时，还提供了数据查询和基本的数据处理功能，即 PromQL 语言
4. 告警模块：右上角的 AlertManager 提供了告警的功能，包括基于 PromQL 语言的触发条件、告警规则的配置管理以及告警的发送等
5. 可视化展示模块：Prometheus 的 web UI 提供了简单的可视化界面，用于执行 PromQL 查询语句，但结果的展示比较单调。一旦配合 Grafana，就可以构建非常强大的图形界面了

![img](https://static001.geekbang.org/resource/image/e5/91/e55600aa21fd6e8d96373f950b2a9991.png)

![img](https://static001.geekbang.org/resource/image/28/86/28410012526e7f91c93ce3db31e68286.png)

### 应用监控的一般思路

##### 指标监控

应用程序的核心指标，不再是资源的使用情况，而是请求数、错误率和响应时间。下面几种指标，也是监控应用程序时必不可少的

* 第一个，是应用进程的资源使用情况，比如进程占用的 CPU、内存、磁盘 I/O、网络等。使用过多的系统资源，导致应用程序响应缓慢或者错误数升高，是一个最常见的性能问题
* 第二个，是应用程序之间调用情况，比如调用频率、错误数、延时等。由于应用程序并不是孤立的，如果其依赖的其他应用出现了性能问题，应用自身性能也会受到影响
* 第三个，是应用程序内部核心逻辑的运行情况，比如关键环节的耗时以及执行过程中的错误等。由于这是应用程序内部的状态，从外部通常无法直接获取到详细的性能数据。所以，应用程序在设计和开发时，就应该把这些指标提供出来，以便监控系统可以了解其内部运行状态

为了迅速定位这类跨应用的性能瓶颈，可以使用 Zipkin、Jaeger、Pinpoint 等各类开源工具，来构建全链路跟踪系统

##### 日志监控

性能指标的监控，可以迅速定位发生瓶颈的位置，只有指标的话往往还不够。同样的一个接口，当请求传入的参数不同时，就可能会导致完全不同的性能问题。除了指标外，还需要对这些指标的上下文信息进行监控，而日志正是这些上下文的最佳来源。最经典的方法，是 ELK 技术栈，即使用 Elasticsearch、Logstash 和 Kibana 这三个组件的组合

![img](https://static001.geekbang.org/resource/image/c2/05/c25cfacff4f937273964c8e9f0729405.png)

* Logstash 负责对从各个日志源采集日志，然后进行预处理，最后再把初步处理过的日志，发送给 Elasticsearch 进行索引
* Elasticsearch 负责对日志进行索引，并提供了一个完整的全文搜索引擎，可以从日志中检索需要的数据
* Kibana 则负责对日志进行可视化分析，包括日志搜索、处理以及绚丽的仪表板展示等

Logstash 资源消耗比较大。在资源紧张的环境中，使用资源消耗更低的 Fluentd，来替代 Logstash

![img](https://static001.geekbang.org/resource/image/69/e7/69e53058a70063f60ff793a2bd88f7e7.png)

### 分析性能问题的一般步骤

##### 系统资源瓶颈

USE 法，即使用率、饱和度以及错误数这三类指标来衡量。系统的资源，可以分为硬件资源和软件资源两类

##### CPU 性能分析

top、pidstat、vmstat 这类工具所汇报的 CPU 性能指标，都源自 /proc 文件系统（比如 /proc/loadavg、/proc/stat、/proc/softirqs 等）。这些指标，都应该通过监控系统监控起来。虽然并非所有指标都需要报警，但这些指标却可以加快性能问题的定位分析

##### 内存性能分析

同 CPU 性能一样，很多内存的性能指标，也来源于 /proc 文件系统（比如 /proc/meminfo、/proc/slabinfo 等），它们也都应该通过监控系统监控起来

##### 磁盘和文件系统 I/O 性能分析

同 CPU 和内存性能类似，很多磁盘和文件系统的性能指标，也来源于 /proc 和 /sys 文件系统（比如 /proc/diskstats、/sys/block/sda/stat 等）。它们也应该通过监控系统监控起来

##### 网络性能分析

同前面几种资源类似，网络的性能指标也都来源于内核，包括 /proc 文件系统（如 /proc/net）、网络接口以及 conntrack 等内核模块。这些指标同样需要被监控系统监控

##### 应用程序瓶颈

其本质来源，实际上只有三种，也就是资源瓶颈（系统资源瓶颈模块提到的各种方法）、依赖服务瓶颈（使用全链路跟踪系统）以及应用自身的瓶颈（应用程序指标监控以及日志监控）。如果这些手段过后还是无法找出瓶颈，可以用系统资源模块提到的各类进程分析工具，来进行分析定位

* 用 strace，观察系统调用
* 用 perf 和火焰图，分析热点函数
* 用动态追踪技术，来分析进程的执行状态

系统资源和应用程序本来就是相互影响、相辅相成的一个整体。很多资源瓶颈，也是应用程序自身运行导致的。比如，进程的内存泄漏，会导致系统内存不足；进程过多的 I/O 请求，会拖慢整个系统的 I/O 请求等

### 优化性能问题的一般方法

##### 系统优化

1. CPU

核心在于排除所有不必要的工作、充分利用 CPU 缓存并减少进程调度对性能的影响：

* 第一种，把进程绑定到一个或者多个 CPU 上，充分利用 CPU 缓存的本地性，并减少进程间的相互影响
* 第二种，为中断处理程序开启多 CPU 负载均衡，以便在发生大量中断时，充分利用多 CPU 的优势分摊负载
* 第三种，使用 Cgroups 等方法，为进程设置资源限制，避免个别进程消耗过多的 CPU。同时为核心应用程序设置更高的优先级，减少低优先级任务的影响

2. 内存

* 第一种，除非有必要，Swap 应该禁止掉。这样就可以避免 Swap 的额外 I/O ，带来内存访问变慢的问题
* 第二种，使用 Cgroups 等方法，为进程设置内存限制。这样就可以避免个别进程消耗过多内存，而影响了其他进程。对于核心应用，还应该降低 oom_score，避免被 OOM 杀死
* 第三种，使用大页、内存池等方法，减少内存的动态分配，从而减少缺页异常

3. 磁盘和文件系统 I/O

* 第一种，通过 SSD 替代 HDD、或者使用 RAID 等方法，提升 I/O 性能
* 第二种，针对磁盘和应用程序 I/O 模式的特征，选择最适合的 I/O 调度算法。SSD 和虚拟机中的磁盘，通常用的是 noop 调度算法；而数据库应用，更推荐使用 deadline 算法
* 第三，优化文件系统和磁盘的缓存、缓冲区，比如优化脏页的刷新频率、脏页限额，以及内核回收目录项缓存和索引节点缓存的倾向等等

使用不同磁盘隔离不同应用的数据、优化文件系统的配置选项、优化磁盘预读、增大磁盘队列长度等，也都是常用的优化思路

4. 网络

* 首先，从内核资源和网络协议的角度来说，对内核选项进行优化：
  * 增大套接字缓冲区、连接跟踪表、最大半连接数、最大文件描述符数、本地端口范围等内核资源配额
  * 减少 TIMEOUT 超时时间、SYN+ACK 重传数、Keepalive 探测时间等异常处理参数
  * 开启端口复用、反向地址校验，并调整 MTU 大小等降低内核的负担

* 其次，从网络接口的角度来说，对网络接口的功能进行优化：
  * 将原来 CPU 上执行的工作，卸载到网卡中执行，即开启网卡的 GRO、GSO、RSS、VXLAN 等卸载功能
  * 开启网络接口的多队列功能，每个队列就可以用不同的中断号，调度到不同 CPU 上执行
  * 增大网络接口的缓冲区大小以及队列长度等，提升网络传输的吞吐量

* 最后，在极限性能情况（比如 C10M）下，内核的网络协议栈可能是最主要的性能瓶颈，一般会考虑绕过内核协议栈：
  * 使用 DPDK 技术，跳过内核协议栈，直接由用户态进程用轮询的方式，来处理网络请求。再结合大页、CPU 绑定、内存对齐、流水线并发等多种机制，优化网络包的处理效率
  * 使用内核自带的 XDP 技术，在网络包进入内核协议栈前，就对其进行处理。也可以达到目的，获得很好的性能

##### 应用程序优化

性能优化的最佳位置，还是应用程序内部，在观察性能指标时，应该先查看应用程序的响应时间、吞吐量以及错误率等指标，因为它们才是性能优化要解决的终极问题：

* 第一，从 CPU 使用的角度来说，简化代码、优化算法、异步处理以及编译器优化等，都是常用的降低 CPU 使用率的方法，这样可以利用有限的 CPU 处理更多的请求
* 第二，从数据访问的角度来说，使用缓存、写时复制、增加 I/O 尺寸等，都是常用的减少磁盘 I/O 的方法，这样可以获得更快的数据处理速度
* 第三，从内存管理的角度来说，使用大页、内存池等方法，可以预先分配内存，减少内存的动态分配，从而更好地内存访问性能
* 第四，从网络的角度来说，使用 I/O 多路复用、长连接代替短连接、DNS 缓存等方法，可以优化网络 I/O 并减少网络请求数，从而减少网络延时带来的性能问题
* 第五，从进程的工作模型来说，异步处理、多线程或多进程等，可以充分利用每一个 CPU 的处理能力，从而提高应用程序的吞吐能力

除此之外，还可以使用消息队列、CDN、负载均衡等各种方法，来优化应用程序的架构，将原来单机要承担的任务，调度到多台服务器中并行处理。这样也往往能获得更好的整体性能

一定要避免过早优化。性能优化往往会提高复杂性，一方面降低了可维护性，另一方面也为适应复杂多变的新需求带来障碍。性能优化最好是逐步完善，动态进行；不追求一步到位，要首先保证，能满足当前的性能要求。发现性能不满足要求或者出现性能瓶颈后，再根据性能分析的结果，选择最重要的性能问题进行优化