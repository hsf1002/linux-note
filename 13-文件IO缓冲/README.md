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

![image-20191113065350117](/Users/sky/Library/Application Support/typora-user-images/image-20191113065350117.png)

