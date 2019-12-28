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

##### exec库函数

```
#include <unistd.h>

int execl(const char *path, const char *arg, ...);
int execlp(const char *file, const char *arg, ...);
int execle(const char *path, const char *arg,..., char * const envp[]);
int execv(const char *path, char *const argv[]);
int execvp(const char *file, char *const argv[]);
int fexecve(int fd, char *const argv[],char *const envp[]);

int execve(const char *file, char *const argv[],char *const envp[]);
```

- 第一个参数是文件名、文件描述符或文件路径，当path作为参数时，如果包含/，视为路径名，否则按照PATH环境变量指定的目录搜索可执行文件
- 后缀l表示列表list，v表示矢量vector，前三个函数的每个命令行参数都是一个独立的参数，而后四个函数应先构造一个指向各参数的指针数组，再将其作为参数传递
- 以e结尾的三个函数可以传递一个指向环境字符串指针数组的指针，其他四个函数则是使用调用进程中的environ变量为新程序复制现有的环境
- 字母p表示该函数取path作为参数，并且用PATH环境变量寻找可执行文件
- l与v互斥，p与e互斥，p与f互斥
- 进程中每个打开描述符都有一个执行时关闭标志FD_CLOEXEC，若设置了此标志，执行exec时关闭该描述符
- exec执行前后实际用户ID和实际组ID保持不变，而有效ID是否改变取决于所执行程序文件的设置用户ID位和设置组ID位是否设置，如果设置了，则有效用户ID变成程序文件所有者ID，组ID处理方式一样
- 在大多UNIX实现中，只有execve是内核的系统调用，其余六个只是库函数

##### 解释器脚本

解释器：能够读取并执行文本格式命令的程序，如UNIX shell、awk、sed、perl、python和ruby的程序，除了能够交互式读取和执行命令外，还可以从脚本文件中读取和执行命令

* 必须赋予脚本文件可执行权限
* 文件的起始行必须指定脚本解释器的路径名，一般是绝对路径

```
#! interpreter-path [optional-arg]
```

