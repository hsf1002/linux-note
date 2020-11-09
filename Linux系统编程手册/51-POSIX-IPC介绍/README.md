## 第51章 POSIX IPC 介绍

### API概述

![WechatIMG47.jpeg](https://i.loli.net/2020/02/06/lB79mQDe5EgTJHo.jpg)

##### IPC对象名字

SUSv3规定的唯一一种标识POSIX IPC对象的可移植的方式是使用以斜线打头后面跟一个或多个非斜线字符的名字，如：/myobject

##### 创建或打开一个IPC对象

IPC open调用返回的句柄与传统的open返回的文件描述符类似，具体来说，消息队列返回mqd_t，信号量返回sem_t *指针，共享内存返回一个文件描述符

##### 关闭IPC对象

对于消息队列和信号量而言，调用相应的close可以释放之前与该对象关联的所有资源，共享内存的关闭通过munmap解除映射完成

##### IPC对象权限

与文件的权限掩码一样，对于IPC对象而言，执行权限没有意义

##### IPC对象删除和对象持久性

与System V IPC对象不同，POSIX IPC对象有引用计数，所以可以安全的删除一个对象，每个类型的IPC对象都有对应的unlink会立即删除对象的名字，当引用计数为0时销毁该对象

与System V IPC对象相同，POSIX IPC对象拥有内核持久性，一旦被创建，就一直存在直到被断开或系统关闭，这样一个进程创建对象，修改其状态，然后退出，后面的进程可以访问该对象

##### 通过命令行列出和删除IPC对象

System V IPC可以通过ipcs和ipcrm来列出和删除IPC对象，POSIX IPC没有此类命令，Linux中，IPC对象是通过挂载在根目录/下某处的真实或虚拟文件系统实现，因此可以使用标准的ls和rm列出和删除IPC对象

##### Linux编译POSIX IPC的程序

需要连接实时库librt，cc命令可以指定-lrt选项

### System V IPC与POSIX IPC比较

POSIX IPC的优势：

* 接口简单
* 对象引用计数
* 使用名字替代键，open、close、unlink与传统UNIX文件模型一致

System V IPC的优势：

* 可移植性