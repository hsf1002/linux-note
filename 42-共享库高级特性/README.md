### 第42章 共享库高级特性

##### 动态加载库

有时候延迟加载库比较有用，dlopen API使得程序可以在运行时打开一个共享库，根据名字在库中搜索一个函数，然后调用这个函数，这种在运行时才加载的共享库称为动态加载的库，它的创建方式与其他共享库完全一样

* dlopen：打开一个共享库
* dlsym：在库中搜索一个符号（函数或变量）并返回其地址
* dlclose：关闭一个共享库
* dlerror：调用上述函数时返回一个错误消息字符串

Linux上使用dlopen API必须指定-ldl选项以便与libdl库链接起来

##### 打开共享库：dlopen

将名字为libfilename的共享库加载到调用进程的虚拟地址空间并增加该库的打开引用计数：

```
#include <dlfcn.h>

void *dlopen(const char *libfilename, int flags);
// 返回值：若成功，返回库的句柄，若出错，返回NULL
// 如果libfilename包含斜线/，将被解析为绝对或相对路径名
// 如果libfilename依赖其他库，则自动加载那些库
// 同一个库文件中可以多次调用dlopen，只会加载进内存一次，但是引用计数会增加

flags是位掩码，取值如下：
RTLD_LAZY：只有当代码被执行的时候才解析库中未定义的函数符号，延迟加载只适用于函数引用，变量引用会立刻解析
RTLD_NOW：在dlopen结束之前立刻加载库中所有的未定义引用，打开库变慢，但能检测潜在错误，一般调试时使用
RTLD_GLOBAL：这个库及其依赖树中的符号在解析由这个进程加载的其他库中的引用和通过dlsym查找时可用
RTLD_LOCAL：默认值，与RTLD_GLOBAL含义相反
RTLD_NODELETE：在dlclose中不要卸载库，即使引用计数是0，后续dlopen时不会重新初始化库中的静态变量，gcc -Wl, -znodelete含义类似
RTLD_NOLOAD：不加载库
RTLD_DEEPBIND：在解析这个库中的符号引用时优先搜索库中的定义，再搜索已加载的库中的定义，-Bsymbolic含义类似
```

##### 错误诊断：dlerror

如果调用dlopen或其他API出现错误，可以通过dlerror获取错误原因的指针：

```
const char *dlerror(void);
// 返回值，若成功，返回错误提示字符串，若从上次调用dlerror到现在没有发生错误，返回NULL
```

##### 获取符号地址：dlsym

在handle指向的库以及该库的依赖树的库中搜索名字为symbol的符号（函数或变量）：

```
void *dlsym(void *handle, char *symbol);
// 返回值：若成功，返回符号地址，若未找到，返回NULL
// handle可以取值如下伪句柄
RTLD_DEFAULT：从主程序开始查找symbol，接着按序在所有已加载的共享库中查找
RTLD_NEXT：在调用dlsym之后加载的共享库中搜索symbol，适用于需要创建与在其他地方定义的函数同名的包装函数的情况
```

C99禁止函数指针与void*之间的赋值操作：

```
funcp = dlsym(handle, symbol);
```

应该改为如下的类型转换：

```
*(void **)(&funcp) = dlsym(handle, symbol);
```

