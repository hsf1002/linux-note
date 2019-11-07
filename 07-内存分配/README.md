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

malloc的实现很简单，首先会扫描之前由free释放的空闲内存块列表，以求找到尺寸大小大于或等于要求的一块内存，如果找到了就返回，如果找不到则调用sbrk以分配更多的内存；当malloc分配内存时，会额外分配几个字节用来存放这块内存大小，这个整数值位于内存的起始处，而实际返回给调用者的内存地址恰好位于这个整数地址的后面，因此free可以获知要释放的内存的大小

应该遵守的规则：

* 分配一块内存后，应该小心谨慎，不要改变这块内存范围外的任何内容
* 释放同一块内存超过一次是错误的，除了报段错误外，更多会导致不可预知的后果
* 若非malloc返回的指针，不能在free时使用
* 编写长时间运行的程序，如果需要反复分配内存，应该确保释放所有已经使用完毕的内存，否则堆将稳步增长直至达到可用虚拟地址的上限，然后分配内存的任何尝试都会失败，即产生了内存泄漏

realloc可以调整（通常是增大）一块内存的大小：

```
void *realloc(void *ptr, size_t size);
// 返回值：若成功，返回新分配的内存地址（与之前的地址可能不同），若出错，返回NULL（ptr指向的内存地址不变）
// 一般情况下，应该避免使用realloc
```

calloc用来给一组相同对象分配内存：

```
void *calloc(size_t numitems, size_t size);
// 返回值：若成功，返回新分配的内存起始地址，且将内存初始化为0，若出错，返回NULL
// numitems为对象个数，size为每个对象的大小
```

memalign和posix_memalign的目的在于分配内存时，起始地址要与2的整数次幂边界对其，该特征对某些程序特别有用：

```
#include <malloc.h>

void *memalign(size_t boundary, size_t size);
// 返回值：若成功，返回新分配的内存起始地址，若出错，返回NULL
// 分配size大小的内存，起始地址是boundary的整数倍，而boundary必须是2的整数次幂
```

```
#include <stdlib.h>

int posix_memalign(void **memptr, size_t alignment, size_t size);
// 返回值：若成功，返回0，若出错，返回一个正数
```

##### 在堆栈上分配内存：alloca

alloca是通过增加栈帧的大小从堆栈上分配内存：

```
#include <alloca.h>

void *alloca(size_t size);
// 总是返回分配的内存地址
// 不需要也不能调用free释放由alloca分配的内存，也不能调用realloc调整由alloca分配的内存大小
// 优点是分配内存的速度快于malloc，且其内存会随着栈帧的移除自动释放，在longjmp和siglongjm执行非局部跳转时其作用尤其突出
```

