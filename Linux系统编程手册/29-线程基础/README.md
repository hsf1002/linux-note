## 第29章 线程基础

### 概述

![WechatIMG32.jpeg](https://i.loli.net/2020/01/04/iAKhGnTpbxdaktM.jpg)

进程的弊端：进程间的信息难以共享，调用fork创建进程的代价相对较高，而线程之间能够方便、快捷的共享信息，创建线程比创建进程块10倍甚至更多，每个线程都包含表示执行环境所必须的信息，其中有进程中标识线程的线程ID、一组寄存器值、栈、调度优先级和策略、信号屏蔽字、errno变量以及线程私有数据等

- 通过为每种事件类型分配单独的处理线程，可以简化处理异步事件的代码
- 多个进程必须使用操作系统提供的复杂机制才能实现内存和文件描述符的共享
- 有些问题可以分解从而提高程序的吞吐量
- 交互的程序通过多线程可以改善响应时间

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

### Pthread API详细背景

##### 数据类型

---

pthread_t：tid

pthread_mutex_t：互斥对象

pthread_mutexattr_t：互斥对象属性

pthread_cond_t：条件变量

pthread_condattr_t：条件变量属性

pthread_key_t：线程特有数据的键

pthread_once_t：一次性初始化控制上下文

pthread_attr_t：线程的属性对象

##### 线程和errno

---

Linux将errno定义为宏，可展开为函数，并返回一个左值，为每个线程所独有，errno机制保留传统UNIX API报错方式的同时，适应了多线程环境

##### Pthreads API函数返回值

---

0表示成功，正值表示失败，失败时的返回值，与传统UNIX系统调用errno的含义相同

##### 编译Pthreads程序

---

添加cc -pthread的编译选项，效果如下：

* 定义_REENTRANT预处理宏
* 程序会与库libthread进行链接（-lpthread）

### 创建线程

```
#include <pthread.h>

int pthread_create(pthread_t * __restrict tidp,
		const pthread_attr_t * __restrict attr,
		void * (*start_rtn)(void *),
		void * __restrict arg);
// 返回值：若成功，返回0，若失败，返回错误编号		
```

- 若成功返回，新创建线程的ID会被设置为tidp指向的内存单元
- 新线程从start_rtn开始运行，该函数只有一个无类型指针参数，如果需要多个参数，需要放到一个结构体中
- 线程创建后，不能保证新线程先运行，还是调用线程先运行

### 终止线程

单个线程的退出方式：

- 从启动例程中返回，返回值是线程的退出码
- 被同一进程的其他线程调用pthread_cancel取消
- 调用pthread_exit
- 任意线程调用exit或主程序执行了return

```
void pthread_exit(void * rval_ptr);
```

其返回值可被另一线程通过pthread_join获取，rval_ptr指向的内容不应该分配于线程栈上

### 线程ID

```
pthread_t pthread_self(void);
// 返回值：调用线程的线程ID
```

线程ID只有在它所属的进程上下文中才有意义，必须用函数比较两个线程ID：

```
#include <pthread.h>

int pthread_equal(pthread_t tid1, pthread_t tid2);
// 若相等，返回非0，否则，返回0
```

Linux中，线程ID在所有进程中都是唯一，POSIX线程ID由线程库负责分配和维护，而gettid返回的线程ID是由内核分配的数字，类似于进程ID

### 连接已终止的线程

pthread_join等待由pthread标识的线程终止，如果线程已经终止，则立即返回，否则一直阻塞，直到指定的线程调用pthread_exit、被取消（rval_ptr指定的内存单元设置为PTHREAD_CANCELED）或从启动例程返回，这种操作称为连接

```
int pthread_join(pthread_t thread, void ** rval_ptr)
// 返回值：若成功，返回0，若出错，返回错误编号
```

如果线程没有分离，则必须进行连接，如果未能连接，则线程终止时将产生僵尸线程，pthread_join与waitpid的差异：

* 线程之间的关系时对等的
* 无法连接任意线程，也不能以非阻塞方式进行连接

### 线程的分离

默认情况下，线程是可连接的，即当线程退出时，其他线程可以通过调用pthread_join获取其返回状态，如果不关心其返回状态，希望线程终止时能够自动清理并移除，可以调用pthread_detach将线程标记为分离状态

```
int pthread_detach(pthread_t tod);
// 返回值：若成功，返回0，否则，返回错误编码
```

- 默认线程的终止状态会保存直到调用pthread_join
- 如果线程已经被分离，其底层存储资源可以在线程终止时立即被收回
- 如果线程已经被分离，调用pthread_join会产生未定义行为，也无法使其返回可连接的状态

### 进程VS线程

多线程的优点：

1. 线程间数据共享很简单
2. 创建线程快于进程创建

多线程的缺点：

1. 需要确保调用线程安全的函数，或者以线程安全的方式调用函数
2. 每个线程都在争用宿主进程的有限的虚拟地址空间

权衡与选择：

1. 多线程中处理信号，需要小心设计，一般不建议在多线程程序中使用信号
2. 所有线程都必须运行同一程序
3. 除了数据，线程还可以共享某些其他信息（文件描述符、信号处置、当前工作目录、可用户ID、组ID等）

进程和线程的接口异同：

```
进程原语        线程原语            描述
fork        pthread_create      创建新的控制流
exit        pthread_exit        从控制流中退出
waitpid     pthread_join        得到退出状态
atexit      pthread_cancel_push 注册退出函数
getpid      pthread_self        获取控制流ID
abort       pthread_cancel      请求控制流的非正常退出
```



