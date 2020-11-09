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

##### 文件描述符和打开文件之间的关系

多个文件描述符指向同一个打开文件，既有可能，也属必要，这些文件描述符可能在相同或不同的进程中打开，由内核维护的3个数据结构：

1. 进程级的文件描述符表
   * 控制文件描述符操作的一组标志，目前仅有close-on-exec
   * 对打开文件句柄的引用
2. 系统级的打开文件表
   * 当前文件的偏移量
   * 打开文件使用的状态标记
   * 文件访问模式
   * 与信号驱动IO相关的设置
   * 对该文件i-node对象的引用
3. 文件系统的i-node表
   * 文件类型和访问权限
   * 一个指针，指向该文件所持有的锁的列表
   * 文件的各种属性，包括文件大小和不同类型操作相关的时间戳

*** 进程A中的文件描述符1和20都指向同一个打开的文件句柄，可能是通过dup、dup2、fcntl形成的

*** 进程A中的文件描述符2和进程B中的文件描述符2都指向同一个打开的文件句柄，可能是fork后出现，也可能是当一个进程进程通过UNIX域套接字将一个打开的文件描述符传递给另一个进程

*** 进程A的文件描述符0和进程B的文件描述符3分别指向不同的文件句柄，但这些句柄都指向同一个i-node，即指向同一个文件，这是因为每个进程各自对同一个文件发起了open调用，同一个进程打开文件两次，也会发生

上述讨论揭示了以下要点：

* 两个不同的文件描述符，若指向同一个文件句柄，将共享同一个文件偏移量，如果其中一个通过read、write、lseek修改了文件偏移量，另一个文件描述符也会观察到这一变化，不管这两个文件描述符是属于同一进程还是不同进程
* 获取和修改打开的文件标志（如O_APPEND、O_NONBLOCK）可执行fcntl的F_GETFL和G_SETFL，作用域类似上面
* 而文件描述符标志close-on-exec为进程和文件描述符私有，对其修改不会影响同一或不同进程的其他文件描述符

##### 复制文件描述符

```
#include <unistd.h>

int dup(int oldfd);
// 返回值：若成功返回新的文件描述符，系统保证是未使用的编号最小的，若出错，返回-1
```

```
#include <unistd.h>

int dup2(int oldfd, int newfd);
// 返回值：若成功返回新的文件描述符，编号为newfd，若出错，返回-1
// 如果newfd之前已经打开，会先将其关闭，若知道其已经打开，最好显式关闭
// 如果oldfd无效，则dup2调用失败返回错误EBADF，且不关闭newfd
// 如果oldfd有效，且与newfd相同，那么dup2什么也不做，不关闭newfd，将其返回
```

fcntl的F_DUPFD是复制文件描述符的另一接口，更具灵活性：

```
newfd = fcntl(oldfd, F_DUPFD, startfd)
// 为oldfd创建一个副本，且将大于等于startfd的最小未用值作为描述符编号
```

dup3完成的工作与dup2相同，只是增加了一个flag，只支持一个标记O_CLOEXEC：

```
#include <unistd.h>

int dup3(int oldfd, int newfd, int flags);
// 返回值，若成功返回新的文件描述符，编号为newfd，若出错，返回-1
```

##### 文件特定偏移量处的IO：pread和pwrite

类似于read和write，只是会在offset指定的位置进行IO操作，而不是当前偏移量处，而且不会改变文件的当前偏移量

```
#include <unistd.h>

ssize_t pread(int fd, void *buf, size_t count, off_t offset);
// 返回值：若成功，返回读取的字节数，若到文件尾，返回0，若出错，返回-1
ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset);
// 返回值：若成功，返回写入的字节数，若出错，返回-1
```

pread功能等同于如下，但是可以保证原子性：

```
off_t orig;

orig = lseek(fd, 0, SEEK_CUR);
lseek(fd, offset, SEEK_SET);
s = read(fd, buf, len);
lseek(fd, orig, SEEK_SET);
```

##### 分散输入和集中输出：readv和writev

原子性时readv和writev的重要属性

```
#include <sys/uio.h>

ssize_t readv(int fd, const struct iovec *iov, int iovcnt);
// 从fd从读取连续的字节，将其分散填入iov指定的缓冲区
ssize_t writev(int fd, const struct iovec *iov, int iovcnt);
// 将iov指定的所有缓冲区的数据拼接起来，然后以连续的字节序列写入fd中

struct iovec {
    void      *iov_base;      /* starting address of buffer */
    size_t    iov_len;        /* size of buffer */
};
```

Linux2.6.30新增了两个系统调用，preadv和pwritev，将分散输入/集中输出和指定文件偏移量的IO集于一身，它们并非标准的系统调用，但是获得了现代BSD的支持：

```
#include <sys/uio.h>

ssize_t preadv(int fd, const struct iovec *iov, int iovcnt, off_t offset);
ssize_t pwritev(int fd, const struct iovec *iov, int iovcnt, off_t offset);
```

##### 截断文件：truncate和ftruncate

```
#include<unistd.h>

int truncate(const char *path, off_t length);
int ftruncate(int fd, off_t length);
// 返回值：若成功，返回0，若出错，返回-1
// 若当前长度大于length，调用将丢弃超出部分，若小于length，将在文件尾添加一系列空字节或文件空洞
// 如果是符号链接，会进行解引用
// 该系统调用不会修改文件偏移量
// 调用truncate时无需用open打开文件
```

##### 非阻塞IO

打开文件时指定O_NONBLOCK目的有二：

1. 若open未能立即打开文件，返回错误，而非陷入阻塞，有一种情况下例外，调用open操作FIFO
2. 调用open成功后，后续的IO操作也是非阻塞的

管道、FIFO、套接字、设备（终端和伪终端）都支持非阻塞模式

内核保证了普通文件IO不会陷入阻塞，故而打开普通文件一般会忽略O_NONBLOCK标志，当使用强制文件锁时，该标志对普通文件也是起作用的

Linux定义了O_NDELAY，但含义与O_NONBLOCK一样

##### 大文件IO

32位中，文件偏移量的数据类型off_t的大小是2GB的限制，应用程序可用两种方式获得LFS功能（Large File Summit）

1. 使用支持大文件操作的备选API，这些过渡性扩展，已经过时（如open64、lseek64、stat64等）
2. 编译程序时将宏_FILE_OFFSET_BITS定义为64，这一方法更为可取，有两种做法：
   * 编译选项：cc -D_FILE_OFFSET_BITS=64 prog.c
   * 源文件所有头文件之前添加定义：#define _FILE_OFFSET_BITS 64

##### /dev/fd目录

对于每个进程，内核都提供一个特殊的虚拟目录/dev/fd，包含如/dev/fd/n的文件名，n是与文件描述符对应的编号，如/dev/fd/0即标准输入；/dev/fd实际是符号链接，链接到/proc/self/fd目录，程序中很少使用/dev/fd，主要用途在shell

##### 创建临时文件

生成唯一的文件名并打开文件，返回可用于IO的文件描述符：

```
#include <stdlib.h>

int mkstemp(char *template);
// 返回值：若成功，返回文件描述符，若出错，返回-1
// 模板参数是路径名形式，最后6个字符必须是XXXXXX，这6个字符会被替换以保证文件名的唯一性
```

一般如下使用：

```
int fd;
char template[] = "/tmp/somestringXXXXXX";

if (-1 == (fd = mkstemp(template)))
	perror("mkstemp error");
printf("file name is %s\n", template);
unlink(template);

if (-1 == close(fd))
	perror("close error");
```

使用tmpnam、tempnam、mktemp也能生成唯一的文件名，但是会导致程序出现安全漏洞，应避免使用

tmpfile会创建名称唯一的临时文件，并以读写方式打开（使用了O_EXCL标志，防止其他进程已经创建同名文件）：

```
#include <stdio.h>

FILE *tmpfile(void);
// 返回值：若成功，返回文件流，若出错，返回NULL
// 文件流关闭时将自动删除文件，而tmpfile会在打开文件后，从内部立即调用unlink删除该文件名
```