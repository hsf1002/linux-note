## 第50章 虚拟内存操作

### 改变内存保护：mprotect

修改起始位置为addr长度为len字节的虚拟内存区域分页的的保护：

```
#include <sys/mman.h>

int mprotect(void *addr, size_t len, int prot);
// 若成功，返回0，若出错，返回-1
// addr必须是sysconf(_SC_PAGESIZE)的整数倍
// prot是PROT_NONE或PROT_READ、PROT_WRITE、PROT_EXEC三个的一个或多个的或
```

进程在访问一块内存区域时违背了内存保护，就会收到SIGSEGV信号，用途之一是修改通过mmap设置的映射内存区域的保护

### 内存锁：mlock和mlockall

将一个进程的虚拟内存的部分或全部锁进内存确保他们总是位于物理内存的原因：

* 提高性能，永远不会因为分页故障发生延迟
* 安全，包含敏感信息的虚拟内存分页永远不会被交换出去，就不会写入到磁盘

##### RLIMIT_MEMLOCK资源限制

* 特权进程能够锁住的内存数量没有限制
* 非特权进程能够锁住的内存数量上限由RLIMIT_MEMLOCK定义

RLIMIT_MEMLOCK会影响到mlock和mlockall、mmap MAP_LOCKED、shmctl SHM_LOCK

* 对于mlock和mlockall、mmap MAP_LOCKED：RLIMIT_MEMLOCK定义的是进程级别的限制
* 对于shmctl SHM_LOCK：RLIMIT_MEMLOCK定义的是用户级别的限制

##### 给内存区加锁或解锁

```
int mlock(void *addr, size_t len);
int munlock(void *addr, size_t len);
// 若成功，返回0，若出错，返回-1
```

Linux特有的/proc/PID/status的VmLck可以查看当前进程已经锁住的内存数量

除了显式的调用munlock外，以下情况也会导致内存锁被自动删除：

* 进程终止时
* 被锁住的分页通过munmap被解除映射时
* 被锁住的分页使用mmap MAP_FIXED 标记的映射覆盖时

##### 内存加锁语义的细节

内存锁不会通过fork继承，也不会在exec时保留；多个进程共享一组分页时如MAP_SHARED，只要还有一个进程持有这些分页的内存锁，就会保持被锁进内存的状态；如果一个进程重复的在一个特定虚拟地址区域调用mlock，只会建立一个锁

##### 给一个进程占据的所有内存加锁和解锁

```
int mlockall(int flag);
int munlockall(void);
// 若成功，返回0，若出错，返回-1
// flag的取值：
MCL_CURRENT: 将调用进程的虚拟地址空间中当前所有映射的分页锁进内存，包括程序文本段、数据段、内存映射和栈
MCL_FUTURE: 将后续调用进程的虚拟地址空间中所有分页锁进内存
```

### 确定内存驻留性：mincore

查询在一个虚拟地址范围内哪些分页当前驻留在RAM，因此访问时不会导致分页故障：

```
#define _BSD_SOURCE
#include <sys/mman.h>

int mincore(void *addr, size_t len, unsigned char *vec);
// 若成功，返回0，若出错，返回-1
// 返回起始地址为addr长度为len字节的虚拟地址范围内的内存驻留信息，通过vec返回
```

### 建议后续的内存使用模式：madvise

madvise通过通知内核调用进程对起始地址为addr长度为len的范围分页可能的使用情况来提升程序性能

```
#define _BSD_SOURCE
#include <sys/mman.h>

int madvise(void *addr, size_t len, int advise);
// 若成功，返回0，若出错，返回-1
// advise的取值：
MADV_NORMAL：默认行为，分页以簇的形式传输，导致一些预先读和事后读
MADV_RANDOM：会被随机访问
MADV_SEQUENTIAL：只会被访问一次，且时顺序访问，内核会激进的预先读，并在访问之后就将分页释放了
MADV_WILLNEED：预先读此区域以备将来的访问只需
MADV_DONTNEED：调用进程不再要求此区域的分页驻留在内存中
```

