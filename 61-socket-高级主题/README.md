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