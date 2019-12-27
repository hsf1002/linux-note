### 第27章 程序的执行

##### 执行新程序：execve

```
#include <unistd.h>

int execve(const char* pathname, char* const argv[], char *const envp[]);
// 返回值：若成功不返回，若出错，返回-1
// pathname既可以是绝对路径，也可以是相对路径，通常应该等于basename
// argv指定了传递的参数
// envp指定了新程序的环境列表
// Linux特有的/proc/PID/exe文件是一个符号链接，包含PID对应进程可执行文件的绝对路径名
```

* 如果pathname指定的文件设置了set-user-ID或set-group-ID权限位，系统会在执行此文件时将进程的有效用户ID或有效组ID设置为程序文件的属主或组ID，利用这个机制，可另用户在运行程序时获得特权

* 无论是否更改了有效用户ID，都会以进程的有效用户ID覆盖已保存的set-user-ID，以有效组ID覆盖已保存的set-group-ID

函数一旦返回，说明出现了错误：

* EACCES：pathname没有指向一个常规文件，未对该文件赋予可执行权限，或某一级目录不可搜索，还有一种可能：以MS_NOEXEC标志挂载文件系统
* ENOENT：pathname指向的文件不存在
* ENOEXEC：系统无法识别其文件格式
* ETXTBSY：一个或多个进程已经以写方式打开pathname指向的文件
* E2BIG：参数列表和环境列表所需空间总和超过了允许的最大值