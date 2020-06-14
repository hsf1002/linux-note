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