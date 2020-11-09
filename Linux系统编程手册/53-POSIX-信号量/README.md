## 第53章 POSIX信号量

### 概述

POSIX信号量有两种：

* 命名信号量：拥有一个名字，不相关的进程能够访问同一个信号量
* 未命名信号量：没有名字，位于内存一个预先商定的位置

POSIX信号量的值是一个整数，不能小于0，如果一个进程试图将其减小到小于0，调用阻塞或返回错误

Linux2.6以上以及带NPTL的glibc上才可用，需要链接 -pthread

### 命名信号量

Linux上，命名信号量被创建为小型POSIX共享内存对象，名字形式为sem.name，挂载在/dev/shm目录下专用tmpfs文件系统，其具有内核持久性

##### 打开一个命名信号量

```
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <semaphore.h>

sem_t *sem_open(const char *name, int oflag, mode_t mode, unsigned int value);
// 若成功，返回指向sem_t的指针，若出错，返回SEM_FAILED
// name标识信号量，一般是sem.name
// 若获取，oflag的取值为0，并忽略后两个参数
// 若创建，oflag的取值为O_CREATE，如果同时指定了O_CREATE和O_EXCL但是信号量已经存在，返回失败
// mode是权限，默认是O_RDWR
// value是无符号整数，信号量初始值，这就可以保证POSIX信号量的创建和初始化原子的
// fork的子进程会继承父进程打开的所有命令信号量的引用
```

##### 关闭一个命名信号量

```
int sem_close(sem_t *sem);
// 若成功，返回0，若出错，返回-1
// 进程终止或执行一个exec时自动关闭信号量
```

##### 删除一个命名信号量

```
int sem_unlink(sem_t *sem);
// 若成功，返回0，若出错，返回-1
```

### 信号量操作

与System V信号量的差别：

* sem_post、sem_wait一次只操作一个信号量
* sem_post只加一、sem_wait只减一
* POSIX信号量没有提供wait-for-zero的操作（sops.sem_op字段指定为0的semop操作）

##### 等待一个信号量

将sem引用的信号量值减一：

```
int sem_wait(sem_t *sem);
// 若成功，返回0，若出错，返回-1
// 如果信号量值大于0，立即返回，等于0，阻塞直到信号量值大于0
// 如果阻塞中被信号中断，就会失败返回EINTR错误，即使信号处理器采用了SA_RESTART标记
```

非阻塞版本：

```
int sem_trywait(sem_t *sem);
// 若成功，返回0，若出错，返回-1
// 如果不能立即执行，就会失败返回EAGAIN错误
```

超时版本：

```
int sem_timedwait(sem_t *sem);
// 若成功，返回0，若出错，返回-1
// 如果超时后不能执行，就会失败返回ETIMEDOUT错误
```

##### 发布一个信号量

将sem引用的信号量值加一：

```
int sem_post(sem_t *sem);
// 若成功，返回0，若出错，返回-1
// 如果信号量值等于0，并且某个进程正在因等待递减这个信号量而阻塞，该进程被唤醒
```

##### 获取信号量当前值

```
int sem_getvalue(sem_t *sem, int *sval);
// 若成功，返回0，若出错，返回-1
// 如果一个或多个进程（或线程）正阻塞，那么sval的返回值取决于实现，Linux返回0
```

### 未命名信号量

也称为基于内存的信号量，与命名信号量接口一样，还需另外两个接口：sem_init和sem_destroy，使用场景：

* 线程间或进程间共享的信号量不需要名字
* 如果正在构建一个动态数据结构如二叉树，且每一项都需要一个关联的信号量，最简单的做法是为每项分配一个未命名信号量

##### 初始化一个未命名信号量

```
int sem_init(sem_t *sem, int pshared, unsigned int value);
// 若成功，返回0，若出错，返回-1
// pshared的取值：
等于0：信号量在调用进程的线程间共享，sem通常是一个全局变量或堆上的变量的地址，此信号量具备进程持久性
不等于0：信号量将会在进程间共享，sem必须是共享内存区域（POSIX共享内存对象、mmap创建的共享映射、System V共享内存段）
// 未命名信号量不存在权限设置
// SUSv3规定对未命名信号量重复初始化导致未定义行为，必须将程序设计为只有一个进程或线程调用sem_init
```

##### 销毁一个未命名信号量

```
 int sem_destroy(sem_t *sem);
 // 若成功，返回0，若出错，返回-1
 // sem必须是之前使用sem_init的未命名信号量
 // 只有不存在进程或线程等待一个信号量时才能安全的销毁此信号量，销毁后可以重新初始化
```

### 与其他同步技术比较

##### POSIX信号量与System V信号量比较

优势：

* 接口简单
* 消除了System V信号量初始化问题
* 将一个POSIX未命名信号量与动态分配的内存对象关联起来更加简单：只要将信号量嵌入到对象中
* 高度频繁争夺信号量的场景中，性能差不多，争夺不频繁的场景中，POSIX信号量性能要好很多

劣势：

* 可移植性差
* 不支持System V信号量的特性特性

##### POSIX信号量与Pthreads互斥体比较

它们的性能比较接近，但是互斥体是首选，因为其所有权属性能够确保代码具有良好的结构性，信号量有时候被称为并发性编程中的goto；信号处理器中可以使用sem_post与另一个线程同步，而互斥体接口并非接口安全，然而处理异步信号的首选方法是使用sigwaitinfo来接收信号，而不是使用信号处理器

### 信号量的限制

SUSv3定义了两个限制：

* SEM_NSEMS_MAX：一个进程能够拥有的POSIX信号量的最大数目
* SEM_VALUE_MAX：一个POSIX信号量值能取的最大值

