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

##### Pthread API详细背景

数据类型

---

pthread_t：tid

pthread_mutex_t：互斥对象

pthread_mutexattr_t：互斥对象属性

pthread_cond_t：条件变量

pthread_condattr_t：条件变量属性

pthread_key_t：线程特有数据的键

pthread_once_t：一次性初始化控制上下文

pthread_attr_t：线程的属性对象

线程和errno

---

Linux将errno定义为宏，可展开为函数，并返回一个左值，为每个线程所独有，errno机制保留传统UNIX API报错方式的同时，适应了多线程环境

Pthreads API函数返回值

---

0表示成功，正值表示失败，失败时的返回值，与传统UNIX系统调用errno的含义相同

编译Pthreads程序

---

添加cc -pthread的编译选项，效果如下：

* 定义_REENTRANT预处理宏
* 程序会与库libthread进行链接（-lpthread）

