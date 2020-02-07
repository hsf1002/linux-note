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







```
int shm_unlink(const char *name);

```

