### 第5章 深入探究文件IO

##### 原子操作和竞争条件

竞争状态是这样一种情形：操作共享资源的两个进程或线程，其结果取决于一个无法预期的顺序

同时指定O_EXCL与O_CREATE时，如果要打开的文件已经存在，返回一个错误，这提供了一种机制：保证进程是打开文件的创建者，对文件是否存在的检查和创建文件属于同一原子操作

多进程同时向一个文件尾部添加数据，要规避竞争状态，需要将文件偏移量的移动和数据写入纳入同一原子操作，在打开文件时加入O_APPEND标志可以保证这点

##### 打开文件的状态标志

fcntl的用途之一是针对一个打开的文件，获取或修改其访问模式和状态标志，如

```
int flags;
int access_mode;

if (-1 == (flags = fcntl(fd, F_GETFL)))
	perror("fcntl get error");
if (flags & O_SYNC)
	printf("write are synchronized");
```

判断文件模式稍微复杂：

```
access_mode = flags & O_ACCMODE;
if (access_mode == O_WRONLY || access_mode == O_RDWR)
	printf("file is writtable");
```

修改文件的状态标志，适用于以下场景：

* 文件不是由调用程序打开，所以程序也无法使用open来控制文件的状态标志
* 文件描述符的获取是通过open之外的系统调用，如pipe或socket

添加O_APPEND标志：

```
flags |= O_APPEND;
if (-1 == fcntl(fd, F_SETFL, flags))
	perror("fcntl set error");
```

