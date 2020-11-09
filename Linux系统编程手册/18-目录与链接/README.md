### 第18章 目录与链接

##### 目录与（硬）链接

目录的存储方式与文件类似，区别有二：

* 在i-node条目中，将目录标记为不同的文件类型
* 目录是经过特殊组织而成的文件，本质上是一个表格，包含文件名和i-node编号

![WechatIMG21.jpeg](https://i.loli.net/2019/11/24/giA4Xk1nSDKUEax.jpg)

i-node编号始于1用来记录文件系统中的坏块，0表示该条目未使用，文件系统的根目录总是存储在i-node条目2中，i-node列表中没有文件名，这样在相同或不同的目录中可以创建多个名称，每个都指向相同的i-node节点，这些名称就是硬链接

```
echo -n 'it is good to collect things,' > abc
ln abc xyz
echo 'but it is better to go on walks.' >> xyz
cat abc
it is good to collect things,but it is better to go on walks.
ls -li abc xyz   // 第三列表示对i-node链接的计数
7284800 -rw-r--r--  2 sky  staff  62 11 24 21:25 abc
7284800 -rw-r--r--  2 sky  staff  62 11 24 21:25 xyz
```

```
rm abc
ls -li abc xyz
ls -li xyz
7284800 -rw-r--r--  1 sky  staff  62 11 24 21:25 xyz
```

仅当i-node链接计数为0时，即移除了文件的所有名字时，才会删除文件的i-node记录和数据块；一个文件描述符指向一个i-node，而这个i-node的文件名则可能有多个甚至一个都没有，所以无法通过文件描述符确定文件名，Linux上可以通过/proc/PID/fd目录内容的扫描，获知一个进程当前打开了哪些文件

对硬链接的限制有二：

1. 硬链接必须与指代的文件驻留在同一文件系统
2. 不能为目录创建硬链接，否则会出现链接环路

##### 符号链接

也称软链接，是一种特殊的文件类型，其数据是另一个文件的名称，因为符号链接指代一个文件，而非i-node编号，所以可以用来链接不同文件系统的一个文件，也可以为目录创建符号链接，符号链接之间可能形成链路，Linux会将对一个完整路径名的解引用总数限制为40次，意在应对超长符号链接链路和环路

![WechatIMG22.jpeg](https://i.loli.net/2019/11/24/jBuU7qh5bpwft4m.jpg)

是否对系统调用的路径名进行解引用，有一点是约定俗成，总是会对路径中的目录部分的符号链接进行解引用，而是否对文件名本身进行解引用，取决于系统调用，大部分操作会无视符号链接的所有权和权限，是否允许操作是由其所指代的文件的所有权和权限决定，仅当带有sticky权限位的目录对符号链接进行移除或改名时，才会考虑符号链接自身的所有权；符号链接可以指向不存在的文件，用 ls -l 可以查看，但是无法cat

```
如果是符号链接：
解引用：access、chdir、chmod、chown、create、exec、link、open、opendir、pathconf、stat、truncate
不解引用：lchown、lstat、readlink、remove、rename、unlink
```

##### 创建和移除硬链接：link和unlink

```
#include <unistd.h>

int link(const char* existingpath, const char* newpath);
int linkat(int efd, const char* existingpath, int nfd, const char* newpath，int flag);
// 两个函数返回值：若成功，返回0，若出错，返回-1
// 这两个函数创建一个新目录项newpath，引用现有文件existingpath，如果newpath存在，返回出错
```

- 任意路径名是绝对路径，则相应的文件描述符被忽略
- 现有文件是符号链接文件，flag参数设置了AT_SYMLINK_FOLLOW标志，就创建指向符号链接目标的链接，否则创建指向符号链接本身的链接
- 创建新目录项和增加链接计数是一个原子操作
- 如果实现支持创建指向一个目录的硬链接，也仅局限于超级用户可以这样做，理由是可能在文件系统中形成循环，大多数文件系统不能处理这种情况，很多系统实现不允许对于目录的硬链接

```
#include <unistd.h>

int unlink(const char* pathname);
int unlinkat (int fd,const char* pathname, int flag);
// 两个函数返回值：若成功，返回0，若出错，返回-1
// 不能移除一个目录
```

- 如果pathname是符号链接，那么unlink删除该符号链接而不是所指向的文件
- 删除目录项并将pathname所引用文件的链接计数减1，如果该文件还有其他链接，仍然可以通过其他链接访问该文件的数据，如果出错，不对文件做任何修改
- 删除文件的条件：链接计数为0并且没有进程打开它
- 关闭文件的条件：打开它的进程数为0并且链接计数为0
- flat提供了一种方法改变unlinkat的默认行为，当AT_REMOVEDIR标志被设置，unlinkat可以类似于rmdir删除目录，如果这个标志被清除，则执行与unlink相同的操作
- unlink的这种特性可以被用来确保即使程序奔溃时，所创建的临时文件也不会遗留下来
- remove解除对一个文件或目录的链接。对于文件，remove与unlink相同，对于目录，remove与rmdir相同

##### 更改文件名：rename

既可以重命名文件，又可以将文件移到同一文件系统下的另一个目录：

```
#include <stdio.h>

int rename(const char *oldname, const char*newname);
int renameat(int oldfd, const char *oldname, int newfd, const char *newname);
// 返回值：若成功，返回0，若失败，返回-1
// 改名既不影响指向该文件的硬链接，也不影响持有该文件打开描述符的任何进程
```

- 如果oldname是一个文件，为该文件或链接文件重命名。如果newname已存在，不能引用一个目录，如果newname已存在且不是一个目录，则先删除该目录项再将oldname重命名为newname，对于包含oldname和newname的目录，调用进程必须具有写权限
- 如果oldname是一个目录，为该目录重命名。如果newname已存在，则必须引用一个空目录（只有.和..），先将其删除，再将oldname重命名为newname。newname不能包含oldname作为其路径前缀
- 如果oldname或newname引用符号链接，则处理符号链接本身
- 不能对.和..重命名
- 如果oldname和newname引用同一个文件，不做任何修改返回

##### 创建并读取符号链接：symlink和readlink

symlink会针对由actualpath指定的路径名创建一个新的符号链接sympath：

```
#include <unistd.h>

int symlink(const char*actualpath, const char *sympath);
int symlinkat(const char *actualpath, int fd, const char *sympath);
// 两个函数返回值：若成功，返回0，若出错，返回-1
// 创建符号链接，并不要求actualpath已经存在，actualpath和sympath也无需位于同一文件系统
```

因为open跟随符号链接（即解引用），需要一种方式打开符号链接本身：

```
ssize_t readlink(const char* restrict pathname, char *restrict buf, size_t bufsize);  
ssize_t readlinkat(int fd, const char* restrict pathname, char *restrict buf, size_t bufsize);
// 两个函数返回值：若成功，返回读取的字节数（不以null字节终止），若出错，返回-1  
// 这两个函数组合了open、read和close的所有操作
```

##### 创建和移除目录：mkdir和rmdir

```
#include <sys/stat.h>     

int mkdir(const char *pathname, mode_t mode);     
int mkdirat(int fd, const char *pathname, mode_t mode);     
// 两个函数返回值：若成功，返回0，若出错，返回-1 
// 创建一个新的空目录，其中.和..自动创建，所指定的文件访问权限mode由进程的文件模式屏蔽字修改
// pathname可以是绝对路径也可以是相对路径，若已经存在，则调用失败并将errno置为EEXIST
// 创建的只是路径名的最后一部分，mkdir("aaa/bbb/ccc", mode)，仅当aaa和bbb已经存在时才会成功
```

```
#include <unistd.h>     

int rmdir(const char *pathname);     
// 返回值：若成功，返回0，若出错，返回-1 
// 要使调用成功，必须是空目录
// 如果pathname的最后一部分是符号链接，不对其进行解引用，调用失败并将errno置为ENOTDIR
// 如果调用此函数使目录的链接计数成为0，并且也没有其他进程打开此目录，则释放由此目录占用的空间
```

##### 移除一个文件或目录：remove

```
#include <stdio.h>

int remove(const char *pathname);
// 返回值：若成功，返回0，若出错，返回-1 
// 如果pathname是文件，则remove调用unlink，如果pathname是目录，则remove调用rmdir
// 如果pathname是符号链接，不进行解引用
```

##### 读取目录：opendir和readdir

对某个目录具有访问权限的任意用户都可以读该目录，但只有内核才能写目录，一个目录的写/执行权限位决定了在该目录能否创建新文件以及删除文件，不代表能否写目录本身

```
#include<dirent.h>

DIR* opendir(constchar * path );
DIR* fdopendir(int fd);
// 两个函数返回值，若成功，返回DIR指针指向目录列表的首条记录，若出错，返回NULL
// opendir会为与目录流相关的文件描述符自动设置close-on-exec标志（FD_CLOEXEC），确保执行exec时自动关闭该文件描述符
```

```
struct dirent *readdir(DIR *dp);	
// 返回值：若成功，返回目录流中下一个目录条目的指针，若出错或在目录尾返回NULL
// 返回时并未对文件名进行排序，使用scandir可以获得经过排序后的文件列表

struct direct
{
    ino_t d_ino;    // i-node编号
    char d_name[];  // NULL字节结尾的文件名
}

int *readdir_r(DIR *dp, struct direct entry, struct dirent **result);	
// 返回值：若成功，返回0，若出错，返回负数
// 是readdir的可重入版本
```

```
void rewinddir(DIR *dp);
// 将目录流回到起点

long telldir(DIR *dp);				
// 返回值与dp关联的目录中的当前位置有关，允许随机访问资源
void seekdir(DIR *dp, long loc);
// 允许随机访问资源
```

```
int closedir(DIR *dp);				
// 返回值：若成功，返回0，若出错，返回-1 
// 关闭由dp指代处于打开状态的目录流，同时释放流所使用的资源
```

```
int dirfd(DIR *dp);
// 返回值：若成功，返回文件描述符，若出错，返回-1 
```

宏__offsetof接受两个参数：结构类型和该结构中某一字段，返回size_t类型的值表示该字段距离该结构起点的字节偏移量，这个宏之所以必要，由于编译器为满足诸如int类型的对齐要求，可能在结构中插入填充字节，这导致结构中某一字段的偏移量可能要大于该属性之前所有字段的长度总和

##### 文件树遍历：nftw

遍历位于文件夹*dirpath*下面的目录树，为每个树的节点调用一次*fn()* ，默认情况下，当前目录总是先于其包含的文件和子目录被处理（先序遍历）；为了避免调用进程的文件描述符被用尽，*nopenfd*指定了 **nftw()** 能够同时打开目录的最大数量。当搜索深度超过这个值，**nftw()** 将会变慢，因为目录必须被关掉和重新打开。**nftw()** 为目录树中的每一层至多使用一个文件描述符

```
#include <ftw.h>

int nftw(const char *dirpath, int (*fn) (const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf),int nopenfd, int flags);
// 
// typeflag的取值如下：
FTW_F：fpath是一个普通文件
FTW_D：fpath是一个目录
FTW_DNR：fpath是一个不能被读的目录
FTW_DP：正在进行后序遍历，fpath是一个目录，并且 flag参数被指定为FTW_DEPTH。（如果flags没有被指定为FTW_DEPTH，那么访问目录时使用的typeflag总会是FTW_D。）路径fpath下的所有文件和子目录已经被处理过了
FTW_NS：在不是符号链接的fpath上调用stat失败，可能的原因是权限问题
FTW_SL：fpath是一个符号链接，flags被设置为FTW_PHYS有效
FTW_SLN：fpath是一个指向不存在的文件的符号链接（只在FTW_PHYS未被设置的时候才会发生）

// flag的取值如下：
FTW_CHDIR：在处理目录内容前先调用chdir进入每个目录，如果func要执行，应该使用这个标志
FTW_DEPTH：进行后序遍历，也就是在处理完当前目录的内容和它的所有子目录之后才会调用fn()
FTW_MOUNT：停留在同一个文件系统中（也就是不会跨越挂载点）
FTW_PHYS：不会跟随符号链接，如果不设置这个flag，就会跟随符号链接，但是没有文件会被报告两次，如果FTW_PHYS没有被设置，但是设置了FTW_DEPTH，那么函数fn() 就永远不会被自己是自己子孙的目录调用到

// func的返回值如下：
FTW_CONTINUE：继续正常进行
FTW_SKIP_SIBLINGS：当前节点的兄弟节点会被跳过，处理从父节点继续进行
FTW_SKIP_SUBTREE：如果pathname是目录，就不对该目录下条目调用func，恢复进行对该目录的下一个同级目录的处理
FTW_STOP：不再进一步处理目录树下任何条目，立即返回FTW_STOP
```

##### 进程的当前工作目录

每个进程都有一个当前工作目录，新进程的当前工作目录继承自其父进程，此目录是搜索所有相对路径名的起点。当前工作目录是进程的一个属性，起始目录则是登录名的一个属性。因为当前工作目录是进程的一个属性，所以它只影响调用chdir的进程本身，而不影响其他进程：

```
#include <unistd.h>

char *getcwd( char *buf, szie_t size );
// 返回值：若成功，返回buf（绝对路径），若出错则返回NULL
// 如果buf是NULL，getcwd将分配一个大小为size的缓冲区，用于向调用者返回结果
// Linux专有符号链接/proc/PID/cwd的内容可以确定任何进程的当前目录
```

```
#include <unistd.h>

int chdir( const char *pathname );
int fchdir( int filedes );
// 两个函数的返回值：若成功，返回0，若出错，返回-1
```

##### 针对目录文件描述符的相关操作

Linux内核提供了一系列新的系统调用，以及一些附加功能，对某些程序非常有用：

```
类似的传统接口       新接口          备注
access          faccessat        支持AT_EACCESS和AT_SYMLINK_NOFOLLOW标志
chmod            fchmodat        
chown            fchownat        支持AT_SYMLINK_NOFOLLOW标志			
stat              fstatat        支持AT_SYMLINK_NOFOLLOW标志
link              linkat         支持AT_SYMLINK_NOFOLLOW标志
mkdir             mkdirat
mkfifo            mkfifoat       基于mknodat库函数
mknod             mknodat
open              openat
readlink         readlinkat
rename            renamtat
symlink           symlinkat
unlink           unlinkat         支持AT_REMOVEDDIR标志
utimes           utimesat         支持AT_SYMLINK_NOFOLLOW标志

以open为例：
#define _XOPEN_SOURCE 700
#include <fcntl.h>

int openat(int dirfd, const char *pathname, int flags, .../* mode_t */);
// 返回值：若成功，返回文件描述符，若出错，返回-1
// 若dirfd是相对路径名，以其作为参照点
// 若pathname是相对路径名，且dirfd是AT_FDCWD，那么pathname是以进程当前工作目录为参照点
// 若pathname是绝对路径名，忽略dirfd
// 若pathname是符号链接，支持AT_SYMLINK_NOFOLLOW标志表示不进行解引用
```

使用新接口的原有有二（以open为例）：

1. 当调用open打开位于当前工作目录之外的文件时，可能发生某些竞争条件
2. 需要针对不同线程拥有不同的“虚拟”目录，将openat与应用维护的目录文件描述符结合，可以模拟这个功能

##### 改变进程的根目录：chroot

每个进程都有根目录，来解释绝对路径（以/开始）时的起点，默认情况下是文件系统的真实根目录：

```
#define _BSD_SOURCE
#include <unistd.h>

int chroot(const char *pathname);
// 返回值：若成功，返回0，若出错，返回-1
// 这会将应用程序限定于文件系统的特定区域，也称设立了一个chroot监禁区
// 可以通过读取Linux专有文件/proc/PID/root符号链接的内容，获取任何进程的根目录
```

* ftp程序时应用chroot的典型事例，用匿名登录ftp时，将使用chroot为新进程设置根目录，一个专门预留给匿名用户的目录

* chroot系统调用从未被视为一个完全安全的监禁措施，首先特权进程可以越狱成功，对于非特权进程也需防范如下的越狱路线：

  * 调用chroot并未改变进程的当前工作目录，因此，通常在调用chroot之前或之后调用一次chdir，如果不这么多，进程就能使用相对路径访问监禁区之外的文件
  * 如果进程对监禁区之外的某一目录持有打开文件描述符，结合fchdir和chroot即可越狱成功，如下所示：

  ```
  int fd;
  
  fd = open("/", O_RDONLY);
  chroot("/home/mtk");  // Jailed
  fchdir(fd);
  chroot(".");    // out of jail
  ```

  为了防止这种情况，必须关闭所有指向监禁区之外目录的文件描述符

* 即使针对上述可能采取了防范措施，仍然可以利用UNIX域套接字传递监禁区之外的文件描述符来越狱

##### 解析路径名：realpath

```
#incude <stdlib.h>

char *realpath(const char *pathname, char *resolved_path);
// 返回值：若成功，返回空字符结尾的字符串，包含符号链接的绝对路径名，若出错，返回NULL
```

##### 解析路径名字符串：dirname和basename

```
#include <libgen.h>

char *dirname(char *pathname);
char *basename(char *pathname);
// 若pathname是/home/mtk/prog.c，dirname返回/home/mtk，basename返回prog.c，若出错，返回NULL
```

![WechatIMG27.jpeg](https://i.loli.net/2019/11/30/gHFOw1SdG7VkxU5.jpg)

