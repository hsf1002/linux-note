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



