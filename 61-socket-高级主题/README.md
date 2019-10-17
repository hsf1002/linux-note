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