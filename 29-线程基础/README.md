### 第29章 线程基础

##### 概述

![WechatIMG32.jpeg](https://i.loli.net/2020/01/04/iAKhGnTpbxdaktM.jpg)

进程的弊端：进程间的信息难以共享，调用fork创建进程的代价相对较高，而线程之间能够方便、快捷的共享信息，创建线程比创建进程块10倍甚至更多

线程之间共享的信息：

* PID和PPID
* PGID与会话ID
* 控制终端
* 进程凭证
* 打开的文件描述符
* 由fcntl创建的记录锁
* 信号处置
* 文件系统相关信息（umask、chroot、chdir）
* 定时器（setitimer和timer_create）
* 资源限制
* CPU时间消耗（由times返回）
* 资源消耗（由getrusage返回）
* nice值（由setpriority和nice设置）

各个线程独享的属性：

* tid
* 信号掩码
* 线程特有数据
* 备选信号栈
* errno变量
* 浮点型环境（fenv）
* 实时调度策略和优先级
* CPU亲和力
* 能力
* 栈，本地变量和函数的调用链接信息

所有的线程驻留同一虚拟地址空间，利用一个合适的指针，各个线程可以在对方栈中相互共享数据