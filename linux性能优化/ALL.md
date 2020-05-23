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

