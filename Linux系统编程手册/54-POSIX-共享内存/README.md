## 第54章 POSIX共享内存

### 概述

POSIX共享内存能够让无关进程共享一个映射区域而无需创建一个相应的映射文件，Linux使用挂载于/dev/shm下的tmpfs文件系统，POSIX共享内存的shm_open和mmap的关系类似于System V共享内存的shmget和shmat的关系，POSIX共享内存对象的引用通过文件描述符完成，因此可直接使用UNIX各种文件描述符相关的系统调用

需要链接-lrt

### 创建共享内存对象

```
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */

int shm_open(const char *name, int oflag, mode_t mode);
// 若成功，返回文件描述符，若出错，返回-1
// name标识共享内存对象
// 若创建，oflag的取值为O_CREATE，如果同时指定了O_CREATE和O_EXCL但是信号量已经存在，返回失败
// 若获取，oflag的取值为O_RDONLY、O_RDWR、O_TRUNC，并忽略后面参数，Linux上截断在只读打开也会发生
// mode是权限，若获取指定为0
// 返回的文件描述符会设置FD_CLOSEXEC，所以程序执行exec时文件描述符会自动关闭
// 新的共享内存对象创建时长度为0，通常需要在mmap之前进行ftruncate设置对象大小
```

### 删除共享内存对象

```
int shm_unlink(const char *name);
// 若成功，返回0，若出错，返回-1
// 删除共享内存对象不会影响对象的既有映射，它会保持直到调用munmap为止，但会阻止后续的shm_open打开此对象，所有的进程都解除映射，这个对象才会删除
```

### 共享内存API比较

System V共享内存、POSIX共享内存对象、共享文件映射

下面的要点适用于所有技术：

* 它们提供了快速的IPC，通常需要一个信号量同步对共享区域的访问
* 一旦共享内存区域被映射到进程的虚拟内存空间，它就和进程的内存空间其他部分没有差异了
* 系统会以类似的方式将共享内存区域放置进程的虚拟内存空间，Linux特有的/proc/PID/maps可查看所有类型的共享内存信息
* 共享内存在不同进程的内存空间地址不相同，对区域的访问要使用偏移量而不是绝对地址指针
* 操作虚拟内存的系统调用如mprotect、mlock、mincore、madvise适用于这三种技术

显著的差异：

* System V共享内存使用键和标识符模型，POSIX共享内存模型与传统UNIX IO模型兼容
* System V共享内存的大小在创建时确定，POSIX共享内存和文件映射在创建后通过ftruncate调整
* System V共享内存的受支持程度最高