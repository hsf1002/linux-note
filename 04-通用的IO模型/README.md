### 第4章 文件IO：通用的IO模型

##### 概述

所有执行IO操作的系统调用都会以文件描述符，一个非负整数，来指代一个打开的文件，包括管道、FIFO、socket、终端、设备和普通文件，针对每个进程，文件描述符都有一套

```
文件描述符    用途    POSIX名称      stdio流
0         标准输入  STDIN_FILENO    stdin
1         标准输出  STDOUT_FILENO   stdout
2         标准错误  STDERR_FILENO   stderr
```

##### 通用IO

IO模型的特点是其输入输出的通用性，4个系统调用open、read、write、close可以对所有类型的文件执行IO操作

```
./copy oldfile newfile	// 拷贝普通文件
./copy a.txt /dev/tty   // 拷贝普通文件到当前terminal
./copy /dev/tty b.txt   // 拷贝当前terminal到普通文件
./copy /dev/pts/16 /dev/tty // 从另一个terminal拷贝到当前terminal
```

##### 打开文件：open

```
#include <fcntl.h> 
 
int open(const char *path, int oflag,... /* mode_t mode */);   
int openat(int fd, const char *path, int oflag, ... /* mode_t mode */ );  
// 两函数的返回值：若成功，返回文件描述符；若出错，返回−1 
```

若未指定O_CREATE标记，则可以省略mode参数，即创建文件，必须指定mode设置权限默认值，但是权限，不仅依赖于mode，还受到进程umask和父目录的默认访问控制列表影响：

- O_RDONLY：只读打开
- O_WRONLY：只写打开
- O_RDWR：读写打开
- O_EXEC：只执行打开
- O_SEARCH：只搜索打开（目录）

oflag必须指定上面至少一个，下面选项是可选

- O_APPEND：追加
- O_CREATE：不存在则创建
- O_DIRECTORY：如果path引用的不是目录，则出错
- O_EXCL：如果同时指定了O_CREATE，文件已经存在，则出错，可以测试一个文件是否存在，使得测试和创建成为一个原子操作
- O_CLOEXEC：多线程中执行fcntl的F_GETFL和F_SETFL会导致竞争状态，而该标记可以避免这点，
- O_SYNC：每次write等待物理I/O操作完成，数据和属性同步更新
- O_ASYNC：当对open返回的文件描述符实施IO操作时，系统会产生一个信号通知进程，仅对特定类型的文件如终端、FIFO以及socket有效，Linux中，open指定该标志无效，必须调用fcntl的F_SETFL来设置
- O_TRUNC：如果文件存在，且为只写/读写，打开成功，将其长度切断为0
- O_DSYNC：每次write等待物理I/O操作完成，如果写操作不影响，刚写入的数据，则不需等待文件属性被更新
- O_RSYNC：使得每个以文件描述符为参数进行的read操作等待，直至所有对文件同一部分挂起的写操作都完成
- O_LARGEFILE：在32位系统打开大文件
- O_NOATIME：调用read，不修改文件的最近访问时间
- O_NOCTTY：不要让path成为控制终端
- O_NOFOLLOW：对符号链接不予解引用
- O_NONBLOCK：以非阻塞方式打开

这些常量可分为三组：文件访问模式标记（可以通过fcntl的F_GETFL进行检索）、文件创建标记（不可以通过fcntl的F_GETFL进行检索或修改）、已打开文件的状态标记（可以通过fcntl的F_GETFL和F_SETFL进行检索和修改）

由open和openat函数返回的文件描述符一定是最小的未用的数值，常量_POSIX_NO_TRUNC决定是否要截断过长的文件名或路径名，还是返回一个错误

出错时open返回-1，错误号errno标识错误原因：

* EACCES：权限问题，不允许以flag指定的方式打开文件
* EISDIR：企图以打开文件方式打开目录
* EMFILE：进程已打开的文件描述符数量达到了进程资源限制设定的上限
* ENFILE：文件打开数量达到了系统允许的上限
* ENOENT：要么文件不存在且未指定O_CREATE标志，要么指定了O_CREATE标志，但path指定的目录之一不存在，或者为符号链接，但指向的是空链接
* EROFS：指定的文件属于只读文件系统，企图以写方式打开
* ETXTBSY：指定的文件为可执行文件，且正在运行，系统不允许修改正在运行的程序

```
int create(const char *pathname, mode_t mode);	
// 出错返回-1
等同于：
open（path, O_WRONLY|O_CREAT|O_TRUNC, mode);
```

##### 读文件：read

```
// 返回已经读到的字节数，若到文件尾，返回0，出错返回-1
ssize_t read(int fd, void * buf, size_t count);		
```

有多种情况可能导致实际读到的字节数小于要求读的字节数：

- 普通文件：读到要求的字节数前到达文件尾
- 从终端设备读取：通常一次只能读一行
- 从网络读取：网络的缓冲机制可能造成返回值小于要求读的
- 从管道或FIFO读取：若管道包含的字节小于要求读的
- 当一个信号造成中断，而已经读了部分数据量时

##### 写文件：write

```
// 返回已经写入的字节数，出错返回-1
ssize_t write (int fd, const void * buf, size_t count);
```

部分写的原有可能是磁盘已满，或进程资源对文件大小的限制RLIMIT_FSIZE

##### 关闭文件：close

```
int close(int fd);
```

当一个进程终止时，内核自动关闭它所有打开的文件，文件描述符属于有限资源，关闭失败可能导致一个进程将文件描述符资源耗尽，所以也需要进行错误检查

##### 文件偏移量：lseek

默认打开一个文件时，除非指定O_APPEND，否则偏移量为0，并不适合所有文件，如管道、FIFO或网络套接字或终端

```
off_t lseek(int fd, off_t offset, int whence);	// 成功返回偏移量，出错返回-1
```

- SEEK_SET：距离文件开始处的offset个字节
- SEEK_CUR：距离当前偏移量的offset个字节
- SEEK_END：距离文件结尾处的offset个字节

------

- 如果文件描述符指向一个管道、FIFO或网络套接字，返回-1，且errno为ESPIPE
- 测试lseek的返回值时需要谨慎，不要测试是否小于0，要测试是否等于-1
- lseek仅将文件偏移量记录到内核，不引起任何IO操作，该偏移量用于下一次读写
- 文件偏移量可以大于文件长度，对文件下一次的写操作将加长该文件并构成一个空洞（文件大小一样，但是占用的磁盘块不一样），没写过的字节被读为0

 ##### 文件空洞

如果程序的文件偏移量已经跨越了文件结尾，再执行IO操作，read会返回0，表示文件结尾，而write会写入成功，从文件结尾到重新写入的这段空间为文件空洞，文件空洞存在字节，但是不占用磁盘空间，核心转储文件coredump是文件空洞的常见例子。文件空洞的存在意味着一个文件名义上的大小可能比实际磁盘总量要大（甚至大很多），向文件空洞写入字节，内核需要分配存储单元，即使文件大小不变，系统的可用磁盘空间也会减小