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

使用utimensat和futimens改变文件时间戳：

utimensat系统调用和futimens库函数对修改文件的上次访问时间和上次修改时间提供了扩展功能：

* 可以精确到纳秒级别
* 可独立设置某一时间戳
* 可独立将任意时间设置为当前时间

```
#define _XOPEN_SOURCE 700  // or define _POSIX_C_SOURCE >= 200809
#include<sys/stat.h>

int utimensat(int dirfd, const char *path, const struct timespec times[2], int flag);
// 返回值：若成功，返回0，若出错，返回-1
// 若将某一时间戳设置为当前时间，将tv_nsec指定为UTIME_NOW，且忽略tv_sec字段
// 若将某一时间戳保持不变，将tv_nsec指定为UTIME_OMIT，且忽略tv_sec字段
// 若dirfd指定为AT_FDCWD，对pathname的解读与times类似
// 也可将dirfd指定为某目录的文件描述符，目的如18.11节描述
// flag可是0或AT_SYMLINK_NOFOLLOW，表示当pathname是符号链接时不进行解引用

struct timespec
{
	  time_t tv_sec;
	  long   tv_nsec;
}
```

如下代码将文件atime设置为当前时间，同时mtime保持不变：

```
struct timespec t[2];

t[0].tv_sec = 0;
t[0].tv_nsec = UTIME_NOW;
t[1].tv_sec = 0;
t[1].tv_nsec = UTIME_OMIT;
if (-1 == utimensat(AT_FDCWD, "file", t, 0))
	perror("utimeensat error");
```

使用futimens可更新打开文件描述符fd所指代的atime和mtime：

```
#include<sys/stat.h>

int futimens(int fd, const struct timespec times[2]);
// 返回值：若成功，返回0，若出错，返回-1
```

##### 文件属主

装配ext2文件系统时，mount命令选项如果是：

* -o grpid或-o bsdgroups：新建文件总是继承父目录的组ID，忽略父目录的set-group-ID位
* -o nogrpid或-o sysvgroups：新建文件的组ID取自进程的有效组ID，如果父目录设置了set-g位，则组ID继承自父目录

改变文件属主的方法：

```
#include <unistd.h>

int chown(const char *pathname,uid_t owner,gid_t group);
int fchown(int fd,uid_t owner,gid_t group);
int fchownat(int fd,const char *pathname,uid_t owner,gid_t group,int flag)
int lchown(const char *pathname, uid_t owner, gid_t group);     
// 4个函数的返回值：若成功，返回0；若出错，返回-1 
// 如果两个参数owner或group任意一个是-1，则对应的ID不变
// 只有特权进程才能改变文件的用户ID，非特权进程可使用chown将文件组ID修改为其附属组任意组ID，前提是进程的有效用户ID与文件的用户ID匹配
// 如果文件组的属主或属组发生了改变，那么set-user-ID和set-group-ID权限位也会关闭
```

##### 文件权限

stat结构的st_mod字段的低12位定义了文件权限，前三位是set-user-ID、set-group-ID和sticky位，后9位是权限掩码

```
常量      其他值      权限位
S_ISUID  04000     set-user-ID
S_ISGID  02000     set-group-ID
S_ISVTX  01000     sticky

S_IRUSR  0400      user-read
S_IWUSR  0200      user-wriet
S_IXUSR  0100      user-execute

S_IRGRP   040       group-read
S_IWGRP   020       group-wriet
S_IXGRP   010       group-execute

S_IROTH   04       other-read
S_IWOTH   02       other-wriet
S_IXOTH   01       other-execute
```

通常将各类掩码定义为常量如：S_IRWXU(0700)、S_IRWXG(070)、S_IRWXO(07)

目录权限与文件权限相比，有三种另有所指：

* 读：可列出目录之下的内容，如通过ls命令
* 写：在目录内创建、删除文件（删除文件，对文件本身不需要有任何权限）
* 可执行：可访问目录的文件，也称搜索权限

访问文件/home/mtk/x，需要有/、/home、/home/mtk的可执行权限以及对/home/mtk/x的读权限，若有对目录的读权限，可以查看目录的文件列表，要访问文件内容或文件的i节点信息，还需要对目录的可执行权限；若拥有对目录的可执行权限无读权限，只要知道目录的文件名，依然可以访问，但不能列出目录的文件列表，在控制对公共目录内容的访问时，这是简单实用的技术

权限检查算法：

只要在访问文件或目录的系统调用中指定了路径，内核就会检查相应文件的权限，如果路径包含目录前缀，则会检查每个目录的可执行权限，内核会使用文件系统用户ID和组ID来进行文件权限检查，一旦open打开了文件，后续的read、write、fcntl等将不会进行任何权限检查，规则如下：

1. 特权进程，授予所有访问权限
2. 进程有效用户ID与文件的用户ID（属主）相同，根据文件的属主权限，授予相应权限
3. 进程的有效组ID或任一附属组ID与文件的组ID（属组）相同，根据文件的属组权限，授予相应权限
4. 以上三点都不满足，内核根据文件的other权限，授予相应权限

若组权限超过了属主权限，那么文件属主所拥有的权限要低于组成员的权限：

```
echo "hello" > a.txt
ls -al a.txt 
-rw-r--r--  1 sky  staff  6 11 23 16:48 a.txt
chmod u-rw a.txt  // 文件属主删除读写权限
ls -al a.txt	  
----r--r--  1 sky  staff  6 11 23 16:48 a.txt
cat a.txt 	// 文件属主无法访问
cat: a.txt: Permission denied

su avr		// 切换用户，此用户属于文件的属组
Password:
groups:
users staff teach cs
cat a.txt 
hello
```

系统调用access是根据进程的真实用户ID和组ID取检查对文件的访问权限，对于set-user-ID或set-group-ID程序就是这样的：

```
#include <unistd.h>

int access(const char *pathname, int mode);		
int faccessat(int fd, const char *pathname, int mode, int flag);	
// 两个函数的返回值：若成功，返回0；若出错，返回-1 
// 若pathname是符号链接，则进行解引用
// mode值可以为：F_OK（存在） R_OK（具有读权限） W_OK（具有写权限） X_OK（具有执行权限）
// flag参数设置为AT_EACCESS，则访问检查的是进程的有效用户ID和有效组ID
// 出于安全考虑，建议杜绝使用此调用，当文件是符号链接时有漏洞
```

set-group-ID有两种用途：对于以nogrpid选项装配的目录下新建的文件，控制其群组从属关系15.3.1节，以及用于强制锁定文件55.4节

sticky位以前是将其文本内容拷贝到交换区为了提高程序执行的加载速度，现代Linux系统中它所起的作用已经改变，为目录设置该位，表明仅当非特权进程具有对目录的写权限且为文件的属主时，才能对目录下的文件进行删除unlink或rmdir和重命名rename操作，可借此机制来创建为多个用户共享一个目录，各个用户可在其下创建或删除属于自己的文件，但不能删除其他用户的文件，为/tmp设置此位，原因正是如此

```
ll a.txt 
----r--r--  1 sky  staff  6 11 23 16:48 a.txt
chmod +t a.txt 
ll a.txt 
----r--r-T  1 sky  staff  6 11 23 16:48 a.txt
chmod o+x a.txt 
ll a.txt 
----r--r-t  1 sky  staff  6 11 23 16:48 a.txt*
```

umask是一种进程属性，通常继承自父shell，用于指明屏蔽哪些权限位，大多数默认值是八进制的022（---w--w--），如果open中的mode是0666（rw-rw-rw），则新建文件实际权限是rw-r--r--，如果mode是0777（rwxrwxrwx），则新建文件的实际权限是rwxr-xr-x

```
#include <sys/stat.h>

mode_t umask(mode_t cmask);	
// 返回值：之前的文件模式屏蔽字

屏蔽位		含义
------------------------------
0400		用户读
0200		用户写
0100		用户执行
0040		组读
0020		组写
0010		组执行
0004		其他读
0002		其他写
0001		其他执行

read: 4
write: 2
execute: 1

umask值	 文件权限	  文件夹权限
--------------------------------
0			rw			rwx
1			rw			rw
2			r			rx
3			r			r
4			w			wx
5			w			w
6			x			x
7		no permission allowed
```

修改文件权限：

```
#include <sys/stat.h>

int chmod(const char *path, mode_t mode);
#define _XOPEN_SOURCE 500 // or #define _BSD_SOURCE
int fchmod(int fd, mode_t mode);
int fchmodat(int fd, const char *pathname, mode_t mode, int flag);
// 三个函数返回值：成功返回0，出错返回-1
// fchmodat：当pathname为绝对路径时，或者fd参数取值为AT_FDCWD而pathname为相对路径时，等同于chmod。否则，计算相对于由fd指向的打开目录的pathname，当flag设置为AT_SYMLINK_NOFOLLOW时，fchmodat不会跟随符号链接
// 为了改变一个文件的权限位，进程的有效用户ID必须等于文件的所有者ID，或者该进程必须具有超级用户权限

mode 			说明
--------------------------------
S_ISUID			执行时设置用户ID
S_ISGID			执行时设置组ID
S_ISVTX			保存正文（粘着位）

S_IRWXU			用户（所有者）读写执行
	S_IRUSR		用户（所有者）读
	S_IWUSR		用户（所有者）写
	S_IXUSR		用户（所有者）执行

S_IRWXG			组读写执行
	S_IRGRP		组读
	S_IWGRP		组写
	S_IXGRP		组执行

S_IRWXO			其他读写执行
	S_IROTH		其他读
	S_IWOTH		其他写
	S_IXOTH		其他执行
```

若要修改某个权限位：

```
struct stat sb;
mode_t mode;

if (-1 == stat("myfile", &sb))
    perror("stat error");
// user打开写权限，other关闭读权限
mode = (sb.st_mode | S_IWUSR) & ~S_IROTH;
if (-1 == chmod("myfile", mode))
    perror("chmode error");
```

等同于：

```
chmod u+w,o-r myfile
```

##### i节点标志

某些Linux文件系统允许为文件和目录设置各种各样的标志，如ext2，程序中利用ioctl系统调用，shell中利用chattr和lsattr命令来设置和查看i节点标志：

```
lsattr myfile
--------- myfile
chattr +ai myfile  // 打开append和immutable标志
----ia-- myfile
```

```
int attr;

if (-1 == ioctl(fd, FS_IOC_GETFALGS, &attr))
	perror("ioctl get error");

attr |= FS_NOATIME_FL;

if (-1 == ioctl(fd, FS_IOC_SETFLAGS, &attr))
	perror("ioctl set error")
```

