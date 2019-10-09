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





