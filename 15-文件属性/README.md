### 第15章 文件属性

##### 获取文件信息：stat

```
#include <sys/stat.h>

int stat(const char *restrict pathname, struct stat * restrict buf);
int fstat(int fd, struct stat* buf);
int lstat(const char *restrict pathname, struct stat * restrict buf);
int fstatat(int fd, const char * restrict pathname, struct stat *restrict buf, int flag);
// 所有4个函数的返回值：若成功，返回0；若出错，返回-1
// stat和lstat无需对操作的文件拥有任何权限，但针对pathname的父目录要有执行权限

struct stat 
{
    dev_t st_dev; // 标识文件所驻留的设备
    ino_t st_ino; // 文件的i节点号
    mode_t st_mode; // 标识文件类型和指定文件权限双重作用
    nlink_t st_nlink; // 指向文件的硬链接数
    uid_t st_uid;  // uid
    gid_t st_gid;  // gid
    dev_t st_rdev; // 包含设备的主、辅ID
    off_t st_size; // 文件的字节数
    blksize_t st_blksize; // 针对文件系统的文件进行IO时最优块大小，一般是4096
    blkcnt_t st_blocks; // 分配给文件的总块数，是2，4，8的倍数，如果包含空洞，将小于st_size
    time_t st_mtime;  // 上次修改时间
    time_t st_ctime;  // 上次文件状态改变时间
};
```

dev_t类型记录了主、辅ID，利用宏major和minor获取

```
if ((statbuf.st_mode & S_IFMT) == S_IFREG)
等同于
if (S_ISREG(statbuf.st_mode))
```

```
S_ISREG()	普通文件
S_ISDIR()	目录文件
S_ISCHR()	字符特殊文件
S_ISBLK()	块特殊文件
S_ISFIFO()	管道或FIFO
S_ISLNK()	符号链接
S_ISSOCK()	套接字

S_TYPEISMQ()	消息队列
S_TYPEISSEM()	信号量
S_TYPEISSHM()	共享存储对象
```

##### 文件时间戳

大多数情况下，系统调用会将相关时间戳设置为当前时间，如mkdir会影响到当前文件或目录的atime、ctime和mtime以及父目录ctime和mtime，write会影响当前文件和目录的ctime和mtime，rmdir会影响到父目录的ctime和mtime，但是utime及类似调用会将文件上次访问时间和上次修改时间设置为任意值；其中open的O_NOATIME可降低对磁盘的操作次数，提示某些应用的文件访问性能

使用utime和utimes来改变文件时间戳：

```
#include <utime.h>

int utime(const char *pathname, const truct utimbuf *buf);
// 返回值：若成功，返回0，若出错，返回-1
// 若pathname是符号链接，会解引用
// buf若为NULL，将文件的atime和mtime修改为当前时间
// buf若不是NULL，则将文件的atime和mtime修改为其指定的时间

struct utimbuf
{
	  time_t actime;	// access time
	  time_t modtime; // modify time
}
```

utimes可以提供微秒级别的精度：

```
#include <sys/times.h>

int utimes(const char *pathname, const truct timeval tv[2]);
// 返回值：若成功，返回0，若出错，返回-1
```

futimes和lutimes功能与utimes类似：

```
#include <sys/times.h>

int futimes(int fd, const truct timeval tv[2]);
int lutimes(const char *pathname, const truct timeval tv[2]);
// 两个函数返回值：若成功，返回0，若出错，返回-1
// 若pathname是符号链接，lutimes不会进行解引用
```

