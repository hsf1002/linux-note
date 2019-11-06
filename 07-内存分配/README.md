### 第7章 内存分配

##### 在堆上分配内存

进程可以通过增加堆的大小来分配内存，堆是一段长度可变的连续虚拟内存，始于进程的未初始化数据段末尾，随着内存的分配和释放而增减，通常堆的当前内存边界称为program break

![img](https://timgsa.baidu.com/timg?image&quality=80&size=b9999_10000&sec=1573089773621&di=424cce2d0665748b8f35ee4a7ba7f305&imgtype=jpg&src=http%3A%2F%2Fimg2.imgtn.bdimg.com%2Fit%2Fu%3D2206095412%2C2525429976%26fm%3D214%26gp%3D0.jpg)

传统的UNIX提供了两个操作program break的系统调用，在Linux中依然可用：

```
#include <unistd.h>

int brk(void *end_data_segment);
// 返回值：若成功，返回0，若出错，返回-1
// 若成功，会将program break设置为end_data_segment指定的地方，如果试图将其设置为低于初始值的位置，会导致不可预期的行为

void sbrk(intptr_t increment);
// 返回值：若成功，返回0，若出错，返回-1
// 若成功，将program break的原有位置地址，并将program break在原有位置地址上增加increment传入的大小
// 调用sbrk(0)返回program break的当前位置
```

在大多数硬件架构上，malloc是基于8字节或16字节边界进行内存分配的，且分配的内存没有初始化：

```
#include <stdlib.h>

void *malloc(size_t size);
// 返回值：若成功，返回分配空间的起始地址，若出错，返回NULL
// Linux中，malloc(0)返回一小块可用空间
```

一般情况下，free并不降低program break的位置，而是将这块内存添加到空闲内存列表中，供malloc循环调用：

```
void free(void *ptr);
// ptr应该是之前malloc返回的地址
// 如果ptr是空指针，则free什么也不做
// 如果ptr非空，free后再次使用，如调用free两次，将产生不可预知的后果
```

对于那些分配了内存并在进程终止前使用的程序而言，通常会省略free调用，这在程序中分配了多块内存的情况下特别有用，因为多次调用free会消耗大量的CPU时间



