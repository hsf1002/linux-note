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

