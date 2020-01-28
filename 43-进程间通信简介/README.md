## 第43章 进程间通信简介

### IPC工具分类

![WechatIMG41.jpeg](https://i.loli.net/2020/01/28/bie8fHyDFWOmXtK.jpg)

### 各种IPC工具的标识符和句柄

![WechatIMG40.jpeg](https://i.loli.net/2020/01/28/uQPt7qUgHWyd46m.jpg)

### 各种IPC工具的可访问性和持久性

![WechatIMG39.jpeg](https://i.loli.net/2020/01/28/O1ZLlKgTIozsHRV.jpg)

##### 可访问性

* 管道、FIFO、socket的可访问性时根据相关的文件权限掩码确定，虽然System V IPC对象并不位于文件系统，但每个对象有一个相关的权限掩码，其语义与文件的权限掩码类似
* 一些IPC工具如管道、匿名内存映射被标记为只允许相关进程访问，“相关”是通过fork关联
* 为了给文件加锁，进程必须拥有该文件的文件描述符（即打开此文件的权限）
* POSIX未命名信号量的可访问性通过包含该变信号量的共享内存区域的可访问性确定
* Internet domain socket的访问没有限制

##### 持久性

* 进程持久性：只要进程持有IPC对象，该IPC对象的生命周期就不会终止
* 内核持久性：只有显式的删除内核持久的IPC对象或系统关闭时，该对象才会销毁
* 文件系统持久性：会在系统重启的时候保持其中的信息，直到被显式的删除，唯一具备文件系统持久性的IPC对象是基于内存映射文件的共享内存

