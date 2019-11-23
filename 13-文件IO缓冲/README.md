### 第13章 文件IO缓冲

##### 文件IO的内核缓冲：缓冲区高速缓存

read和write系统调用在操作磁盘文件时不会直接发起磁盘访问，而仅仅在用户空间缓冲区和内核缓冲区高速缓存之间复制数据，这样设计使得read和write操作更为快捷，无需等待，减少了内核必须执行的磁盘传输次数；无论是让磁盘写1000次，每次一个字节，还是一次写入1000个字节，内核访问磁盘的字节数是系统的，但是1000次的系统调用耗费的时间总量也是很可观的；一般情况下，缓冲区大小设置为4096字节，可以达到最优性能

##### stdio库的缓冲

操作磁盘文件时，缓冲大块数据可以减少系统调用，C的库函数如fprintf、fscanf、fgets、fputs、fputc、fgetc正是这样做的，使用这些库函数可以避免自行处理对数据的缓冲，无论是调用write输出还是read输入

对于标准IO库，缓冲的目的是减少read和write的次数

- 全缓冲：填满IO缓冲区后才进行实际的IO操作
- 行缓冲：当输入或输出遇到换行时才进行实际的IO操作
- 不缓冲：不对字符进行缓冲存储

ISO C要求缓冲具有以下特征：

- 当且仅当标准输入输出不指向交互式设备，才是全缓冲
- 标准错误绝不会时全缓冲

但没说明标准输入输出指向交互式设备时，是行缓冲还是不缓冲，标准错误是行缓冲还是不缓冲，很多系统默认：

- 标准错误不带缓冲
- 若是指向终端设备的流，行缓冲，否则全缓冲

可以更改缓冲类型：

```
#include <stdio.h>

void setbuf(FILE *restrict fp, char *restrict buf);
int setvbuf(FILE *restrict fp, char *restrict buf, int mode, size_t size);
// 若成功返回0，若出错返回非0

setbuf(fp, buf);
除了不返回结果外，相当于：
setvbuf(fp, buf, (buf != NULL) ? _IOFBF: _IO_NBF, BUFSIZE);
```

- setbuf打开/关闭缓冲机制：打开就必须让buf指向长度为BUFSIZE的缓冲区，默认全缓冲，如果与终端设备相关，可能是行缓冲；关闭就将buf指向NULL
- setvbuf可以通过mode参数说明缓冲类型
  - _IOFBF：全缓冲（磁盘的流默认全缓冲）
  - _IOLBF：行缓冲（终端设备的流默认行缓冲）
  - _IONBF：不缓冲（忽略buf和size，如stderr）

setbuffer允许调用者指定buf缓冲区大小：

```
#define _BSD_SOURCE
#include <stdio.h>

void setbuffer(FILE *fp, char *buf, size_t size);

setbuffer(fp, buf, size);
相当于：
setvbuf(fp, buf, (buf != NULL) ? _IOFBUF : _IONBF, size);
```

无论采取何种缓冲方式，任何时候，都可以调用fflush强制将stdio输出流中的数据（即通过write刷新到内核缓冲区），flush有两层含义：在标准IO库方面，是将缓冲区内容写到磁盘；在终端驱动方面（输入流），表示丢弃已存储在缓冲区的内容

```
#include <stdio.h>

int fflush(FILE *fp);
// 若成功返回0，若出错，返回EOF
```

此函数将该流所有未写的数据传送到内核，如果fp为NULL，导致所有的流被冲洗

关闭相应的流时，将自动刷新其缓冲区

若打开一个流同时用于输入和输出，C99要求：一个输出操作不能紧跟一个输入，两者之间必须调用fflush或定位函数如fseek、fsetpos、rewind；一个输入操作不能紧跟一个输出操作，两者之间必须调用定位函数，除非输入遇到文件尾

##### 控制文件IO的内核缓冲

强制刷新内核缓冲区到输出文件是可能的，而且很有必要，同步IO完成的定义：某一IO操作，要么已成功完成到磁盘的数据传递，要么被诊断为失败

* 同步IO数据完整性：将文件数据内容更新到磁盘

```
int fdatasync(int fd);
// 返回值：若成功，返回0，若失败，返回-1
// 将缓冲数据到刷新到磁盘上，而与fd相关的所有元数据不会马上更新
// Linux提供了sync_file_range提供了比fdatasync更加精准的控制，可以指定刷新区域
```

* 同步IO文件完整性：是IO数据完整性的超集，不仅更新数据内容，还更新文件元数据

```
int fsync(int fd);
// 返回值：若成功，返回0，若失败，返回-1
// 将缓冲数据和与fd相关的所有元数据到刷新到磁盘上
```

Linux实现中，sync调用仅仅将数据刷新到磁盘：

```
void sync(void);
```

如果内容发生变化的内核缓冲区30秒内没有显式同步到磁盘，则长期运行的线程pdflush（Linux2.6）会确保将其刷新到磁盘，文件/proc/sys/vm/dirty_expire_centisecs定义了刷新脏缓冲区的时间间隔

同步所有的写入：O_SYNC标记

```
fd = open(pathname, O_WRONLY | O_SYNC);
```

调用open后，每个write会自动将数据内容和元数据刷新到磁盘，即按照同步IO文件完整性的要求执行，然而，该标记以及fsync、fdatasync、sync的频繁调用对性能的影响极大，应该在设计程序时考虑使用大尺寸的write缓冲区，如4096

* O_DSYNC：同步IO数据完整性的要求执行，如fdatasync
* O_RSYNC：同步IO文件完整性的要求执行，如fsync

打开文件时，同时指定O_RSYNC | O_DSYNC，则按照同步IO数据完整性的要求执行，若同时指定O_RSYNC | O_SYNC，则按照同步IO文件完整性的要求执行

##### IO缓冲小结

首先是通过stdio库将用户数据传递到stdio缓冲区，该缓冲区位于用户态内存区，当缓冲区填满时，stdio库会调用write系统调用，将数据传递到内核高速缓冲区，该缓冲区位于内核态内存区，最终，内核发起磁盘操作，将数据传递到磁盘

![WechatIMG12.jpeg](https://i.loli.net/2019/11/23/fk8r7twjmgQVubP.jpg)

##### 就IO模式向内核提出建议

posix_fadvise系统调用允许进程就自身访问文件数据时可能采取的模式通知内核：

```
#define _XOPEN_SOURCE 600
#include <fcntl.h>

int posix_fadvise(int fd, off_t offset, off_t len, int advice);
// 返回值：若成功，返回0，若出错，返回一个正值
// offset和len确定了建议所适用的文件区域
```

内核可以（但不是必须）根据所提供的信息来优化对缓冲区高速缓存的使用，进而提高进程和整个系统的性能

- POSIX_FADV_NORMAL：进程对访问模式无建议，默认值
- POSIX_FADV_SEQUENTIAL：进程预计会从低偏移量到高偏移量顺序读取数据
- POSIX_FADV_RANDOM：进程预计以随机顺序访问数据，Linux禁用此选项
- POSIX_FADV_WILLNEED：进程预计会在不久访问指定的文件区域，内核将指定数据填充到缓冲区，后续read将不会阻塞，直接从缓冲区抓取
- POSIX_FADV_DONTNEED：进程预计会在不久不会访问指定的文件区域
- POSIX_FADV_NOREUSE：进程预计会一次性的访问指定文件区域，不再复用，Linux中不起作用

##### 绕过缓冲区高速缓存：直接IO

Linux允许在执行磁盘IO时从用户空间直接将数据传递到文件或磁盘设备，称为直接IO或裸IO，对于大多数应用而言，直接IO可能会大大降低性能，因为内核针对缓冲区高速缓存做了不少优化，如顺序读取、在成簇磁盘块上执行IO、允许访问同一文件的多个进程共享高速缓存的缓冲区，应用直接IO无法受益于这些优化，直接IO只适用于有特定IO需求的应用，如数据库系统，其高速缓存和IO优化机制自成一体，无需内核消耗CPU和内存区完成该任务

针对一个文件或块设备，直接执行IO可在open时指定O_DIRECT标记，若一进程以此标记打开了文件，另一进程打开这个文件时没有指定此标记，则直接IO所读写的数据与缓冲区高速缓存的内容之间存在不一致，应避免这一场景，直接IO的限制：

- 用于传递数据的缓冲区，其内存边界必须对齐为块大小的整数倍
- 数据传输的开始点，即文件的偏移量，必须是块大小的整数倍
- 待传递数据的长度必须是块大小的整数倍

块大小指的是设备的物理块大小，通常是512字节

##### 混合使用库函数和系统调用执行文件IO

```
#include <stdio.h>

int fileno(FILE *fp);
// 返回值：若成功，返回文件描述符，若出错，返回-1
FILE *fdopen(int fd, const char *mode);
// 返回值：若成功，返回文件指针，若出错，返回NULL
// mode与fopen时fd对应的mode参数应该一致，否则调用失败
```

一般情况下，printf的输出将在write的后面

```
printf("To man the world is twofold, ");
write(STDOUT_FILENO, "In accordance with his twofold attitude \n", 41);
```

将IO系统调用和库函数混合使用时，使用fflush可以规避此问题，也可以使用setbuf和setvbuf使缓冲区失效，但这样会影响IO性能，因为每个输入输出操作将引起一次write系统调用