### 第61章 socket:高级主题

##### 流式套接字上的部分读和部分写

如果套接字上可用数据比read调用中请求的数据少，可能出现部分读的情况，如果没有足够的缓冲区来传输所有请求的字节，可能出现部分写的情况；如果read返回的字节少于请求的数量，或者阻塞式的write在完成部分数据传输后被信号处理程序中断，有时候需要重新调用系统调用来完成全部数据的传输，自定义的两个函数

```
ssize_t readn(int fd, void *buf, size_t nbytes);
ssize_t writen(int fd, void *buf, size_t nbytes);
// 两个函数返回值：已读、写字节数，若出错则返回-1
```

这两个函数用于循环调用这些系统调用，因此确保了请求的字节数总是能够全部得到传输

##### 专门用于套接字的系统调用：recv()和send()

```
ssize_t send(int sockfd, const void *buf, size_t nbytes, int flags);
// 若成功，返回发送的字节数，若出错，返回-1
// 类似于write，使用send时套接字必须已经连接，buf和nbytes与write含义相同
// send发送成功，只能说明数据已经被无错误的发送到网络驱动程序了，对端不一定接收到了
// 对于字节流协议，send会阻塞直到整个数据传输完毕

// flags的标志由系统实现
MSG_DONTROUTE: 告诉内核，目标主机在本地网络，不用查路由表
MSG_DONTWAIT: 将单个I／O操作设置为非阻塞模式
MSG_OOB:      指明发送的是带外信息
MSG_MORE:     效果如同套接字选项TCP_CORK，对于UNIX域套接字没有效果
MSG_NOSIGNAL: 在已连接的流式套接字上发送数据时对端已经关闭了，指定该标记不会产生SIGPIPE信号
MSG_EOF:      发送数据后关闭套接字的发送端

ssize_t recv(int sockfd, void *buf, size_t nbytes, int flags);
// 若成功，返回数据的字节数，若无可用数据或对方已经结束，返回0，若出错，返回-1

// flags的含义：       
MSG_DONTWAIT：	仅本操作非阻塞 	         	  
MSG_OOB：　　　发送或接收带外数据	        
MSG_PEEK：    窥看下一条消息而不读取  	 
MSG_WAITALL： 等待所有数据 	      	 
MSG_TRUNC：   数据被截断，也返回实际长度  
```

#####  系统调用：shutdown()

```
int shutdown(int sockfd, int how);
// 若成功，返回0，若出错，返回-1
// how的取值，SHUT_RD：关闭连接的读端，对于TCP套接字没意义；SHUT_WR：关闭连接的写端，最常见操作；SHUT_RDWR：连接的读端和写端都关闭

close可以关闭一个套接字，但是只有最后一个活动引用关闭时，close才会释放网络端点，而shutdown允许一个套接字处于不活动状态，和引用它的文件描述符数目无关，还可以方便的关闭双向传输中的一个方向，如读或写
```

执行下面操作，连接依然会保持打开，仍然可以通过fd2在连接上做IO操作：

```
fd2 = dup(sockfd);
close(sockfd);
```

执行下面操作，连接的双向通道都会关闭，通过fd2页无法执行IO操作了：

```
fd2 = dup(sockfd);
shutdown(sockfd, SHUT_RDWR);
```

shutdown并不会关闭文件描述符，要关闭文件描述符，必须另外调用close

##### 系统调用：sendfile()

如果将磁盘上的文件内容，不做修改的传输到套接字上，可以：

```
while ((n = read(diskfilefd, buf, BUF_SIZE)) > 0)
    write(sockfd, buf, n);
```

如果文件很大，可能需要多次调用这两个系统调用，很不高效，可以直接将磁盘文件拷贝到套接字上，而不经过用户空间（buf），这种技术称为零拷贝传输

```
#include <sys/sendfile.h>

ssize_t sendfile(int out_fd, int in_fd, off_t *offset, size_t count);
// 返回值：若成功，返回传输的字节，若出错，返回-1
// out_fd指向套接字，in_fd指向的文件必须可以进行mmap()
// 可以使用此接口将数据从文件传输到套接字上，但是反过来不行，也不能通过此接口在两个套接字之间传输数据
```

如果启用TCP_CORK选项，所有的输出（HTTP首部、页面数据）都会缓冲到一个单独的TCP报文段中，直到满足下面条件：

* 达到报文段的大小上限
* 取消了TCP_CORK选项
* 套接字被关闭
* 启动该选项后，从写入第一个字节开始经历了200毫秒

可以通过setsockopt系统调用来启动或关闭该选项，如果希望将sendfile的零拷贝能力和传输文件时在第一个报文段中包含HTTP首部信息的能力结合起来，就需要启用该选项

##### 获取套接字地址

发现绑定到套接字上的地址：

```
#include <sys/socket.h>

int getsockname(int sockfd, struct sockaddr *localaddr, socklen_t *addrlen);  
// 返回值：若成功，返回0，若出错，返回-1
// 如果套接字绑定到了另一个程序，且套接字文件描述符在经过exec调用后仍然可保留，那么此时就能用该接口获取；如果隐式绑定到Internet域套接字，想获取内核分配的临时端口，此接口也可用
```

内核会在如下情况出现时执行一个隐式绑定：

* TCP套接字执行了connect或listen，但没有bind
* UDP套接字上首次调用sendto时，该套接字之前还没有绑定到地址
* 调用bind时将端口号指定为0，此时bind会为套接字指定一个IP地址，但是内核会选择一个临时端口号

如果套接字已经和对等方连接，可以找到对方的地址：

```
#include <sys/socket.h>

int getpeername(int sockfd, struct sockaddr *peeraddr, socklen_t *addrlen); 
// 返回值：若成功，返回0，若出错，返回-1
// 如果服务器进程由另一个程序调用，而accept是由该程序所执行，那么服务器进程可以继承套接字文件描述符，但accept返回的地址信息不存在了
```

##### 深入探讨TCP协议

TCP的报文格式：

![img](https://timgsa.baidu.com/timg?image&quality=80&size=b9999_10000&sec=1570757705744&di=027389263bd8f7ae4d4461d20e5c44b1&imgtype=0&src=http%3A%2F%2Fimg2018.cnblogs.com%2Fblog%2F1286175%2F201904%2F1286175-20190402155706164-992264429.jpg)

* 源端口号：发送端的端口号

* 目的端口号：接收端的端口号

* 序列号：表示在这个报文段中的第一个数据字节序号

* 确认序列号：仅当ACK标志为1时有效。确认号表示期望收到的下一个字节的序号

* 头部长度：有4位，跟IP头部一样，以4字节为单位。最大是60个字节

* 保留位：6位，必须为0

* 控制位：8个标志位：

  * CWR-拥塞窗口减小标记
  * ECE-显式的拥塞通知回显标记

  * URG-紧急指针有效
  * ACK-确认序号有效
  * PSH-接收方应尽快将这个报文交给应用层
  * RST-连接重置
  * SYN-同步序号用来发起一个连接
  * FIN-终止一个连接

* 窗口大小：16位，接收端发送ACK确认时提示自己可接受数据的空间大小
* 校验和：源机器基于数据内容计算一个数值，收信息机要与源机器数值 结果完全一样，从而证明数据的有效性。检验和覆盖了整个的TCP报文段：这是一个强制性的字段，一定是由发送端计算和存储，并由接收端进行验证的
* 紧急指针：如果设置了URG标记，与序号字段中的值相加表示紧急数据最后一个字节的序号。TCP的紧急方式是发送端向另一端发送紧急数据的一种方式
* 选项与填充：最常见的可选字段的最长报文大小MSS（Maximum Segment Size），每个连接方通常都在一个报文段中指明这个选项。它指明本端所能接收的最大长度的报文段
* 数据：需要传输的实际数据

TCP的序列号和确认机制：

![img](https://timgsa.baidu.com/timg?image&quality=80&size=b9999_10000&sec=1571438501&di=f8a5600f8934728f5805cef299a78849&imgtype=jpg&er=1&src=http%3A%2F%2Fwww.embedu.org%2FColumn%2Fimages%2Fcolumn337-1.jpg)

每个通过TCP连接传输的字节都由TCP协议分配了一个逻辑的序列号，报文的序列号字段设置为该传输方向上的数据段第一个字节的逻辑偏移，这样接收端就可以按照正确的顺序进行重组；为了实现可靠的通信，TCP采用主动确认的方式，TCP接收端收到报文后会发送一个ACK确认消息，该消息的确认序号设置为接收方所期望的下一个数据字节的逻辑序列号，也就是上一个接收字节的序列号加1；TCP发送端会设置一个定时器，超时没有收到确认报文，那么该报文会重新发送

##### TCP协议状态机和状态迁移图

TCP结点以状态机的方式来建模：

![img](https://timgsa.baidu.com/timg?image&quality=80&size=b9999_10000&sec=1570843894958&di=027618ae48b9ada1f3e6c818de18af5d&imgtype=0&src=http%3A%2F%2Fblog.chinaunix.net%2Fphoto%2F32588_101231174229.jpg)

实线表示TCP客户端的状态迁移路径：CLOSED->SYN_SENT->ESTABLISHED->FIN_WAIT_1->FIN_WAIT_2->TIME_WAIT->CLOSED

虚线表示TCP服务器的状态迁移路径：CLOSED->LISTEN->SYN_RECEIVED->ESTABLISHED->CLOSE_WAIT->LAST_ACK->CLOSE

1. LISTEN：侦听并等待对端的TCP连接请求
2. SYN-SENT：发送SYN连接请求后，等待对端回复SYN请求
3. SYN-RECV：收到来自对端的SYN请求，并回复SYN请求后，等待对端响应SYN请求的ACK消息
4. ESTABLISHED：代表连接建立，双方在这个状态下进行TCP数据交互
5. FIN-WAIT-1：发送FIN关闭连接请求后，等待对方响应FIN的ACK消息或者对端的FIN关闭请求
6. FIN-WAIT-2：等待对方FIN关闭请求
7. CLOSE-WAIT：等待本地用户（进程）发送FIN关闭请求给对端
8. CLOSING：当双方同时发送FIN关闭请求时，会进入CLOSING状态，等待对端发送FIN报文的响应ACK消息
9. LAST-ACK：收到对端FIN请求后，回复ACK及FIN并等待对方回复FIN的响应ACK消息，此时进入此状态
10. TIME-WAIT：该状态是为了确保对端收到了FIN请求的ACK响应，默认会等待两倍MSL时长（MSL：Maximum Segment Lifetime，即报文最大生存时间，超过这个时间的报文会被丢弃）

##### TCP连接的建立

![img](https://timgsa.baidu.com/timg?image&quality=80&size=b9999_10000&sec=1570844883034&di=fc38048e6df49c15a056f978a1655de0&imgtype=jpg&src=http%3A%2F%2Fimg1.imgtn.bdimg.com%2Fit%2Fu%3D2327482896%2C3371288432%26fm%3D214%26gp%3D0.jpg)

三次握手的步骤：

1. connect调用导致客户端TCP结点发送一个SYN报文到服务器，告知服务器有关客户端的初始序列号
2. 服务器TCP结点必须确认客户端发送的SYN报文，并告知客户端自己的初始序列号，服务器同时返回SYN和ACK控制位的报文
3. 客户端TCP结点发送一个ACK报文来确认服务器TCP结点的SYN报文

前两个步骤中交换的SYN报文可能包含TCP首部

##### TCP连接的终止

![img](https://timgsa.baidu.com/timg?image&quality=80&size=b9999_10000&sec=1571275733267&di=94e22101ce3a6d1b1347ee81ff70e4e2&imgtype=jpg&src=http%3A%2F%2Fimg2.imgtn.bdimg.com%2Fit%2Fu%3D1196104357%2C1513701754%26fm%3D214%26gp%3D0.jpg)

一般由客户端发起执行close，稍后，服务器也执行一个close调用，TCP协议执行的步骤如下：

1. 客户端主动关闭，发送一个FIN报文给服务器
2. 接收到FIN后，服务器发出ACK报文作为回应；之后在服务器端，任何对read操作都会到文件尾
3. 当服务器关闭自己的连接时，发送FIN报文到客户端
4. 接收到FIN后，客户端发送ACK报文作为响应

TCP连接建立到结束完整的流程图：

![img](https://timgsa.baidu.com/timg?image&quality=80&size=b9999_10000&sec=1571275663736&di=3118fc8cb8fe65edfe44903384e73d53&imgtype=0&src=http%3A%2F%2Fimg.dongcoder.com%2Fup%2Finfo%2F201807%2F20180709120439727521.png)

##### TIME_WAIT状态

此状态的两个目的：

* 实现可靠的连接终止
* 让老的重复的报文段在网络中过期失效，这样在连接新的连接时将不再接收它们

##### 监视套接字：netstat

可以显示系统中Internet和UNIX套接字的状态

```
-a或–all 显示所有连线中的Socket
-A<网络类型>或–<网络类型> 列出该网络类型连线中的相关地址
-c或–continuous 持续列出网络状态
-C或–cache 显示路由器配置的快取信息
-e或–extend 显示网络其他相关信息
-i或–interfaces 显示网络界面信息表单
-l或–listening 显示监控中的服务器的Socket
-M或–masquerade 显示伪装的网络连线
-n或–numeric 直接使用IP地址，而不通过域名服务器
-N或–netlink或–symbolic 显示网络硬件外围设备的符号连接名称
-o或–timers 显示计时器
-p或–programs 显示正在使用Socket的程序识别码和程序名称
-r或–route 显示Routing Table
-s或–statistice 显示网络工作信息统计表
-v或–verbose 显示指令执行过程
-V或–version 显示版本信息
-t或–tcp 显示TCP传输协议的连线状况
-u或–udp 显示UDP传输协议的连线状况
-w或–raw 显示RAW传输协议的连线状况
-x或–unix 此参数的效果和指定”-A unix”参数相同
–ip或–inet 此参数的效果和指定”-A inet”参数相同
```

```
Active Internet connections
Proto Recv-Q Send-Q  Local Address          Foreign Address        (state)    
tcp4       0      0  192.168.3.35.61194     113.132.128.134.hp-pdl ESTABLISHED
tcp4       0      0  192.168.3.35.61193     113.132.128.134.hp-pdl ESTABLISHED
tcp4       0      0  localhost.ibprotocol   localhost.61192        CLOSE_WAIT 
tcp4       0      0  localhost.61192        localhost.ibprotocol   FIN_WAIT_2 
tcp4       0      0  localhost.ibprotocol   localhost.61191        CLOSE_WAIT
```

* Proto：套接字使用的协议
* Recv-Q：套接字接收缓冲区中还未被本地应用读取的字节数
* Send-Q ：套接字发送缓冲区中排队等待发送的字节数
* Local Address：套接字绑定到的地址
* Foreign Address：对端套接字绑定到的地址
* State：当前套接字所处的状态

##### 使用tcpdump监视TCP流量

可以监视所有的TCP/IP数据包流量（TCP报文、UDP报文、ICMP报文）

`src > dst: flags data-seqno ack window urg <option>`

```
sudo tcpdump -t -N 'port 22264'

IP .6030 > 192.168.3.35.22264: UDP, length 1397
IP .beeyond-media > 192.168.3.35.22264: UDP, length 1397
IP 120.244.156.226.21321 > 192.168.3.35.22264: UDP, length 1397
```

* src：源IP地址和端口号
* dst：目的IP地址和端口号
* fags：TCP控制位的组合，S（SYN）、F（FIN）、P（PSH）、R（RST）、E（ECE）、C（CWR）
* data-seqno：数据包的序列号范围
* ack：连接的另一端期望的下一个字节的序列号
* window：这条连接相反方向用于传输的接收缓冲区大小
* urg：报文段在指定的偏移上包含紧急数据
* option：可能包含TCP任意选项

其中，src、dst和flags总是显示，其他字段只在合适的时机显示

```
常见参数：
-a：将网络地址和广播地址转换成名字
-A：以ASCII格式打印出所有分组，并将链路层头最小化，方便去捕获web页面内容
-c num：收到指定数量的分组后，tcpdmp就会停止
-D：列出系统中所有可以用以tcpdump截包的网络接口。显示的接口序号或接口名称可以通过-i指定
-q：快速输出，只输出较少的信息
-w：将结果输出到文件中，输出的文件以.pcap作为后缀，可以在其他平台上用wireshark打开
-r：从指定文件读取数据包，这个数据包一般是通过-w生成的
-s snaplen
从每个分组中读取最开始的snaplen个字节
默认情况下是读取68个字节，对IP、ICMP、TCP和UDP而言已经足够，但是可能阶段名称服务器和NFS信息包的协议信息。
-s 0表示不限制长度，输出整个包。
应该将snaplen设置成到感兴趣的信息的最小长度。否则会增加获得快照的时间和减少缓存的数量
-t：在每一行转储行上省略时间戳显示
-tt：在每一行中输出非格式化的时间戳
-ttt：在每一行输出date处理过后的时间戳
-v
-vv
-vvv
以上三点，输出的信息详细度递增
-i
指定抓取数据包的接口
若未指定则会去抓取-D参数列出的网络接口所所截获的包（本地回环口除外）

不常用参数：
-C file_size:指定用-w参数写入文件的文件大小。
-d：将匹配信息包的代码用汇编格式显示
-dd：将匹配信息包的代码用C语言程序段格式显示
-ddd：将匹配信息包的代码用十进制格式显示
-e：在输出行打印数据链路层的头部信息
-f：将外部internet地址以数字形式打印显示
-F：从指定文件中读取表达式，忽略命令行中给出的表达式
-l：使标准输出变成缓冲行形式，可以把数据导出到文件
-L：列出网络接口的已知数据链路
-m：从文件module导入SMI MIB模块定义
-M：指定TCP-MD5选项的验证码
-b：在数据链路层上选择协议，包括ip、arp、rarp、ipx等协议
-n：不把网络地址换成名字（不进行域名解析，速度更快）
-nn：直接以ip和端口显示
-N：输出主机名中的域名部分
-O：不允许分组匹配代码优化程序
-p：不将网络接口设置为混杂模式
-T：将监听到的包直接解析为指定的类型的报文，常见的类型有rpc、cnfp、snmp
-u：输出未解码的NFS句柄
-X：以十六进制与ASCII方式输出，用于抓取http等明文传输协议
-XX：同上
-B：buffer_size：设置系统捕获缓冲区大小
-K：跳过TCP校验和验证
```

```
抓取包含10.10.10.122的数据包
tcpdump -i ens33 -vnn host 10.10.10.122
抓取包含10.10.10.0/24网段的数据包
tcpdump -i ens33 -vnn net 10.10.10.0/24
tcpdump -i ens33 -vnn net 10.10.10.0 mask 255.255.255.0
抓取包含端口22的数据包
tcpdump -i ens33 -vnn port 22
抓取udp协议的数据包
tcpdump -i ens33 -vnn udp
抓取icmp协议的数据包
tcpdump -i ens33 -vnn icmp
抓取arp协议的数据包
tcpdump -i ens33 -vnn arp
抓取ip协议的数据包
tcpdump -i ens33 -vnn ip proto ip
tcpdump -i ens33 -vnn ip
抓取源ip是10.10.10.122的数据包
tcpdump -i ens33 -vnn src host 10.10.10.122
抓取目标ip是10.10.10.122的数据包
tcpdump -i ens33 -vnn dst host 10.10.10.122
抓取源端口是22的数据包
tcpdump -i ens33 -vnn src port 22
抓取源ip是10.10.10.253且目的端口是22的数据包
tcpdump -i ens33 -vnn src host 10.10.10.122 and dst port 22
抓取源ip是10.10.10.122或者端口是22的数据包
tcpdump -i ens33 -vnn src host 10.10.10.122 or port 22
抓取源ip是10.10.10.122且端口不是22的数据包
tcpdump -i ens33 -vnn src host 10.10.10.122 and not port 22
抓取源ip是10.10.10.2且端口是22，或源ip是10.10.10.65且目的端口是80的数据包。
tcpdump -i ens33 -vnn \(src host 10.10.10.2 and port 22 \) or \(src ip host 10.10.10.65 and prot 80\)
抓取源ip是10.10.10.59且目的端口是22，或者源ip是10.10.10.68且目的端口是80的数据包
tcpdump -i ens33 -vnn '\(src host 10.10.10.59 and dst port 22\) 'or '\(src host 10.10.10.68 and dst prot 80\)'
把抓取的数据包记录存到/tmp/fill文件中，当抓取100个数据包后就退出程序
tcpdump -i ens33 -c 100 -w /tmp/fill
从/tmp/fill记录中读取tcp协议的数据包。
tcpdump -i ens33 -r /tmp/fill tcp
从/tmp/fill记录中读取包含10.10.10.58的数据包。
tcpdump -i ens33 -r /tmp/fill host 10.10.10.58
过滤数据包类型是多播并且端口不是22、不是icmp协议的数据包。
tcpdump -i ens33 ether multicast and not port 22 and 'not icmp'
过滤协议类型是ip并且目标端口是22的数据包
tcpdump -i ens33 -n ip and dst prot 22
tcpdump可识别的关键字包括ip、igmp、tcp、udp、icmp、arp等
过滤抓取mac地址是某个具体的mac地址、协议类型是arp的数据包
tcpdump -i ens33 ether src host 00:0c:29:2f:a7:50 and arp
过滤抓取协议类型是ospf的数据包
tcpdump -i ens33 ip proto ospf
直接在tcpdump中使用的协议关键字只有ip、igmp、tcp、udp、icmp、arp等，其他的传输层协议没有可直接识别的关键字
可以使用关键字proto或者ip proto加上在/etc/protocols中能够找到的协议或者相应的协议编号进行过滤。
更加高层的协议，例如http协议需要用端口号来过滤
过滤长度大于200字节的报文
tcpdump -i ens33 greater 200
过滤协议类型为tcp的数据包
tcpdump tcp
```

##### 套接字选项

设置选项的值：

```
#include <sys/socket.h>

int setsockopt(int sock, int level, int option, const void *val, socklen_t len);
// 返回值：若成功，返回0，若出错，返回-1
// level是选项应用的协议，如果选项是通用的套接字层次选项，level是SOL_SOCKET，否则level设置为协议编号，对于TCP，level是IPPROTO_TCP，对于IP，level是IPPROTO_IP
// val根据选项的不同指向一个数据结构或一个整数
// len指定了val指向对象的大小
// option是选项名，通用选项、层次选项或某种协议特定选项的名字
```

查看选项的值：

```
int getsockopt(int sock, int level, int option, void *val, socklen_t *len);
// 返回值：若成功，返回0，若出错，返回-1
```

在通过exec调用继承了套接字文件描述符的程序中，比如inetd调用的程序，这种情况下，该调用非常有用，因为程序可能并不知道它继承而来的套接字是什么类型

##### 套接字选项：SO_REUSEADDR

最常见的一种用途：避免当TCP服务器重启时，尝试将套接字绑定到当前已经同TCP结点相关联的端口上时出现的EADDRINUSE（地址已使用）错误，通常会在以下两种情况下出现：

* 服务器要么通过close，要么是奔溃而执行了一个主动关闭使得TCP结点处于TIME_WAIT状态，直到2倍MSL超时过期为止
* 服务器创建一个子进程处理客户端连接，服务器终止，子进程继续服务客户端，使得维护的TCP结点使用了服务器的知名端口号

这两种情况下，剩余TCP结点无法接受新的连接，尽管如此，默认情况下大多数TCP实现会阻止新的监听套接字绑定到服务器的知名端口上；尽管有另一个TCP结点绑定到了同一个端口，通过设定SO_REUSEADDR标记选项允许我们将套接字绑定到这个本地端口上，大多数TCP服务器都应该开启这个选项

##### 在accept()中继承标记和选项

多种标记和设定都可以同打开的文件描述符和文件描述相关起来，为套接字设定的多个选项和标记，通过accept返回的新套接字会继承吗？在Linux上，如下属性不会继承：

* 文件描述符相关的状态标记，即通过fcntl的F_SETFL操作的标记，包括O_NONBLOCK和O_ASYNC
* 文件描述符标记，通过fcntl的F_SETFD操作的标记，仅一个FD_CLOEXEC
* 与信号驱动IO相关的文件描述符属性，如fcntl的F_SETOWN以及F_SETSIG

除此之外，其他属性都会继承

##### TCP vs UDP

鉴于TCP的优点，为什么要选择UDP：

1. 不必为每个客户端创建和终止连接，使用UDP传送单条消息的开销比TCP要小
2. 对于简单的响应式通信，UDP的速度比TCP要快，DNS就是例子
3. UDP套接字上可以进行广播和组播处理
4. 某些特定类型的应用如音频流和视频流，不需要TCP提供的可靠性也能工作在可接受的程度内

使用UDP又需要保证可靠性的应用需要自行实现可靠性保证，至少需要序列号、确认机制、丢包重传、重复报文检测，如果需要更高级的如流量控制和拥塞控制，最好直接使用TCP

##### 高级功能：带外数据

流式套接字的一种特性，允许发送端将传送的数据标记为高优先级，在telnet、ftp中使用这个特性来终止之前传送的命令，带外数据的发送和接收要在send和recv中指定MSG_OOB标记，当接收到带外数据时，内核为套接字的属主生成SIGURG信号，采用TCP套接字时，任意时刻最多只有一个字节可被标记为带外数据；在某些UNIX实现中，UNIX域流式套接字支持带外数据，而Linux不支持；如今不提倡使用带外数据，因为在某些情况下他可能是不可靠的，另一种方法是维护两个流式套接字用作通信，一个传输普通数据，一个传输带外数据，这种技术可在任何域的流式套接字中使用

F_SETOWN可以设置一个套接字的所有权，如第三个参数为正，指定的是进程ID，为非-1，代表的是进程组ID，安排进程接收套接字的信号：

```
fcntl(sockfd, F_SETOWN, pid);
```

F_GETOWN可以获得当前套接字所有权：

```
owner = fcntl(sockfd, F_GETOWN, 0);
```

如果owner为正，则等于为接收套接字信号的进程的ID，为负，其绝对值为接收套接字信号的进程组ID

TCP支持紧急标记：在普通数据中紧急数据的位置，如设置套接字选项SO_OOBINLINE，则可以在普通数据中接收紧急数据，判断是否已经到达紧急标记：

```
int sockatmark(int sockfd);
// 返回值：若在标记处，返回1，若不在，返回0，若出错，返回-1
// 当下一个要读取的字节在紧急标记处，返回1
```

##### 高级功能：系统调用sendmsg()和recvmsg()

sendmsg能做到write、send、sendto所做的事情：

```
ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags);
// 若成功，返回发送的字节数，若出错，返回-1

struct msghdr 
{
    void          *msg_name;        // protocol address
    socklen_t      msg_namelen;     // size of protocol address
    struct iovec  *msg_iov;         // scatter/gather array
    int            msg_iovlen;      // elements in msg_iov
    void          *msg_control;     // ancillary data (cmsghdr struct)
    socklen_t      msg_controllen;  // length of ancillary data
    int            msg_flags;       // flags returned by recvmsg()
};
```

recvmsg可以做read、recv和recvfrom所做的事情：

```
ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags);
// 若成功，返回数据的字节数，若无可用数据或对方已经结束，返回0，若出错，返回-1

// flags的含义：       
MSG_CTRUNC:   控制数据被截断
MSG_ERRQUEUE: 接收错误信息作为辅助数据
MSG_EOR:      接收记录结束符
MSG_OOB:      接收带外信息
MSG_TRUNC:    一般数据被截断
```

这两个系统调用主要：

* 可以实现分散-聚合IO的功能，recvmsg可以将接收到的数据放入多个缓冲区，而sendmsg可以调用带有msghdr结构的sendmsg来指定多重缓冲区传输数据

* 可以传送包含特定于域的辅助数据，如下所示

##### 高级功能：传递文件描述符

通过sendmsg和recvmsg，可以在同一台主机上通过UNIX域套接字将包含文件描述符的辅助数据从一个进程传递到另一个进程，以这种方式可以传递任意类型的文件描述符，如从open或pipe返回得到的文件描述符

##### 高级功能：接收发送端的凭证

另一个使用辅助数据的例子：通过UNIX域套接字接收发送端的凭证，凭证由发送端进程的用户ID、组ID、进程ID组成，发送端可能将其设置为相应的实际用户ID、有效用户ID或保存设置ID，这样使得接收端可以在同一台主机进行验证

##### 高级功能：顺序数据包套接字

结合了流式套接字和数据包套接字的功能

* 类似流套接字：面向连接，通过bind、listen、accept、connect调用
* 类似数据报套接字：保留消息边界
* 与流套接字一样而不同数据报套接字的是：顺序数据报套接字之间的通信是可靠的，消息会以无错误、按顺序、不重复的方式传输到对端程序，且可以保证消息会到达

从2.6.4开始，Linux在UNIX域上支持了SOCK_SEQPACKET，在Internet域，UDP和TCP协议都不支持SOCK_SEQPACKET，但是SCTP协议支持

##### 高级功能：SCTP以及DCCP传输层协议

SCTP和DCCP是两个新的传输层协议，可能会变得越来越普及

* SCTP：流控制传输协议，同TCP一样，提供可靠的、双向的、面向连接的传输，与TCP不一样的是预留了消息边界；SCTP的特点是支持多条数据流，这样就允许多个逻辑上的数据流通过一条单独的连接传输

* DCCP：数据报拥塞控制协议，同TCP一样，提供拥塞控制能力，防止由于数据报的快速传输而是网络过载，与TCP不同的是对于可靠性或按序传输并不保证，因而可以让不需要用到这些特性的程序避免承担延迟