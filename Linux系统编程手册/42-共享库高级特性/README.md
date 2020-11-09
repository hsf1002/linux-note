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

##### 关闭共享库：dlclose

会减小handle引用的库的系统计数，如果是0则卸载这个库，同时在依赖树中的库递归执行此过程，当进程终止时会隐式的对所有库执行dlclose：

```
int dlclose(void *handle);
// 返回值：若成功，返回0，若出错，返回-1
```

可以使用atexit或on_exit来设置一个在库被卸载时自动调用的函数

##### 获取与加载的符号相关的信息：dladdr

```
int dladdr(void *address, Dl_info *dlip);
// 返回值：如果地址在库中存在，返回非0值，否则返回0

struct {
const char *dli_fname; // 执行包含address的加载模块的文件名
void *dli_fbase;  // 加载模块的句柄，用作dlsym的第一个参数
const char *dli_sname; // 指向与指定的address最接近的符号的名称
void *dli_saddr;  // 最接近符号的实际地址
size_t dli_size; /* ELF only */ 最接近符号的大小
int dli_bind; /* ELF only */ 最接近符号的绑定属性
int dli_type;    // 最接近符号的类型
};
```

##### 在主程序中访问符号

如果使用dlopen动态加载了一个共享库，然后使用dlsym获取共享库的x函数，接着x又调用y函数；而有时候需要让x调用主程序的y实现，可以使用链接器选项--export-dynamic

```
gcc -Wl, --export-dynamic main.c
或
gcc --export-dynamic main.c
或
gcc -rdynamic main.c
```

##### 控制符号的可见性

如static使得一个符号私有于一个代码模块，gcc提供了一个特有的特性声明：

```
void
__attribute__ ((visibility("hidden")))
func(void)
{
	// code
}
```

hidden使得一个符号对构成共享库的所有源代码文件都可见，对库之外的文件都不可见

##### 链接器版本脚本

版本脚本是一个包含链接器ld执行的指令的文本文件，要使用版本脚本必须要指定链接器选项--version-script

```
gcc -Wl, --version-script, myscriptfile.map
```

版本脚本通常是map后缀，其主要作用是控制符号的可见性和符号的版本化

* 控制符号的可见性：可以控制可能在无意中变成全局变量的符号的可见性，假设三个源文件，vis_common.c、vis_f1.c、vis_f2.c分别定义了函数vis_common、vis_f1、vis_f2且vis_common由另两个函数调用

  ```
  gcc -g -c -fPIC -Wall vis_common.c vis_f1.c vis_f2.c
  gcc -g -shared -o vis.so vis_common.o vis_f1.o vis_f2.o
  ```

  通过命令`readelf --sym --use-dynamic vis.so|grep vis_`查看会发现三个符号vis_common、vis_f1、vis_f2，如果要隐藏vis_common，可以定义版本脚本：

  ```
  vim vis.map
  VER_1
  {
  global:
      vis_f1;
      vis_f2;
  local:
      *
  }
  ```

  golobal表示对库之外的程序可见，而local表示对库之外的程序不可见，默认情况下C全局符号对共享库之外的程序是可见的，接着可以构建共享库：

  ```
  gcc -g -c -fPIC -Wall vis_common.c vis_f1.c vis_f2.c
  gcc -g -shared -o vis.so vis_common.o vis_f1.o vis_f2.o \
  -Wl, --version-script, vis.map
  ```

  再次`readelf --sym --use-dynamic vis.so|grep vis_`查看会发现二个符号vis_f1、vis_f2

* 符号的版本化：允许一个共享库提供同一个函数的多个版本，每个程序会使用它与共享库进行（静态）链接时函数的当前版本，符号版本化可以取代传统的共享库主要和次要版本化模型；

  除了xyz之外，其他符号都隐藏

  ```
  vim sv_lib_v1.c
  #include <stdio.h>
  
  void xyz()
  {
  	printf("v1");
  }
  
  vim sv_v1.map
  VER_1
  {
      global:
      	xyz;
      local:
      	*
  }
  
  gcc -g -c fPIC -Wall sv_lib_v1.c
  gcc -g -shared -o libsv.so sv_lib_v1.o -Wl, --version-script, sv_v1.map
  ```

  接着创建一个程序使用这个库：

  ```
  vim sv_prog.c
  #include <stdlib.h>
  
  int main(void)
  {
  	void xyz(void);
  	
  	xyz();
  	
  	exit(EXIT_SUCCESS);
  }
  ```

  编译运行程序:

  ```
  gcc -g -o p1 sv_prog.c libsv.so
  LD_LIBRARY_PATH=. ./p1
  v1
  ```

  现在修改库中xyz的定义，但是需要确保p1仍然能够使用老版本的函数，为此，必须定义两个版本的xyz：

  ```
  vim sv_lib_v2.c
  #include <stdio.h>
  
  __asm__(".symver xyz_old, xyz@VER_1");
  __asm__(".symver xyz_new, xyz@@VER_2");
  
  void xzy_old(void)
  {
  	printf("v1");
  }
  
  void xyz_new(void)
  {
  	printf("v2");
  }
  
  void pqr(void)
  {
  	printf("v2 pqr")
  }
  ```

  两个.symver的汇编指令将两个函数绑定到了两个不同的版本标签上，第二个.symver使用两个@表示当应用程序与这个共享库进行静态链接时应该使用xyz的默认定义，一个符号的.symver定义中只能有一个@@标记

  ```
  vim sv_v2.map
  VER_1
  {
      global:
      	xyz;
      local:
      	*
  };
  
  VER_2
  {
  	global:
  		pqr;
  }VER_1;
  ```

  新的版本脚本提供了新的版本标签，它依赖于VER_1，接着构建共享库的新版本：

  ```
  gcc -g -c fPIC -Wall sv_lib_v2.c
  gcc -g -shared -o libsv.so sv_lib_v2.o -Wl, --version-script, sv_v2.map
  ```

  现在创建一个新程序p2，它使用xyz的新定义，同时程序p1使用xyz的旧定义

  编译运行程序：

  ```
  gcc -g -o p2 sv_prog.c libsv.so
  LD_LIBRARY_PATH=. ./p2
  v2
  LD_LIBRARY_PATH=. ./p1
  v1
  ```

  使用`objdump -t p1|grep xyz`可以打印出每个可执行文件的符号表，从而显示出两个程序使用了不同的版本标签

##### 初始化和终止函数

可以定义一个或多个在共享库被加载和卸载时自动执行的函数，不管库时自动被加载还是被dlopen接口显式加载，初始化和终止函数都会被执行，gcc的constructor和descontructor特性还能创建主程序的初始化和终止函数

```
void __attribute__ ((constructor))  some_name_load(void)
{
	// initialization
}

void __attribute__ ((destructor))  some_name_unload(void)
{
	// finalization
}
```

早期是通过`_init`和`_fini`函数实现的，但是只能定义一个，目前已经不建议使用了

##### 预加载共享库

通过环境变量LD_PRELOAD来实现，首先会加载这些库，可执行文件会自动使用这些库中文件，并覆盖动态链接器在其他情况下搜索到的同名函数；假如libdemo中存在两个函数x1和x2，而另一个共享库libalt中也存在x1的定义，如果想要覆盖前面的定义：

```
LD_PRELOAD=libalt.so ./prog
```

LD_PRELOAD控制着进程级别的预加载行为，/etc/ld.so.preload控制着系统层面的预加载行为

##### 监控动态链接器：LD_DEBUG

监控动态链接器可以知道它在搜索哪些库，可以通过LD_DEBUG知道，对于由动态链接器隐式加载和使用dlopen动态加载的库都有效，默认情况下它会输出到标准错误上，可以将一个路径名赋值给环境变量LD_DEBUG_OUTPUT将输出重定向

