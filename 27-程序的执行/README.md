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

##### exec与文件描述符

默认由exec的调用程序打开的所有文件描述符在exec执行时会保持打开状态，且在新程序中依然有效，shell利用这一特性为其所执行的程序处理IO重定向

```
ls /tmp > dir.txt
```

以上shell命令执行了如下步骤：

1. 调用fork创建子进程
2. 子shell以描述符1（标准输出）打开文件用于输出
3. 子shell执行程序ls，ls将其结果输出到标准输出，亦即文件中

从安全角度考虑，执行新程序应该关闭不必要的文件描述符，使用close会有局限：

* 某些描述符是库函数打开的，但库函数无法使主程序在执行exec之前关闭文件描述符，所以库函数应该总是为打开的文件设置close-on-exec标志
* 如果exec调用失败可能还需要使描述符保持打开状态，如果这些描述符已经关闭，将它们重新打开几乎不可能

如果设置close-on-exec标志，执行exec时，会自动关闭该文件描述符，如果调用exec失败，则继续保持打开状态；当调用dup、dup2或fcntl为文件描述符创建副本时，总是会清除副本描述符的close-on-exec标志

##### exec与信号

exec会将现有进程的文本段丢掉，也包含调用进程创建的信号处理程序，内核会将对所有已设信号的处置重置为SIG_DFL，而其他信号的处置则保持不变，在调用exec期间，进程信号掩码以及挂起信号的设置都得以保存

##### 执行shell命令：system

```
#include <stdlib.h>

int system(const char *cmdstring);
```

UNIX中，system总是可用的，在其实现中调用了fork、waitpid和exec，有四种返回值：

* 如果cmdstring为空，仅当system命令可用时，返回非0值，否则返回0，这可以确定系统是否支持system函数

- fork失败或waitpid返回除了EINTR之外的出错，则system返回-1，且设置errno以指示错误类型
- 如果exec失败（表示不能执行shell），返回值如同shell执行了exit(127)一样
- 否则三个函数都成功，那么system返回shell的终止状态，格式在waitpid中已说明

使用system而不是使用fork和exec的优点：

* 进行了所需的各种出错处理以及各种信号处理
* 无需处理fork、waitpid和exec的调用细节

设置了用户ID和组ID的程序在特权模式下运行时，绝对不能调用system，以为你shell对操作的控制依赖于各种环境变量，因此这样会不可避免的给系统带来安全隐患

##### system的实现

简化版

------

system内部正确处理信号：

如果调用system的进程还创建了其他子进程，对SIGCHLD的信号处理函数也执行了wait，在shell创建的子进程退出并产生SIGCHLD信号，有机会调用waitpid之前，主程序的信号处理函数可能就会执行，即产生了竞争条件，两个不良后果：

* 调用程序误以为为其所创建的某个子进程终止了
* system函数无法获取其子进程的终止状态

如`system("sleep 20")`在输入中断或退出字符，会发送信号给3个进程：调用system程序的进程、system锁创建的一个shell进程，sleep进程，shell在等待子进程期间会忽略SIGINT和SIGQUIT信号，默认会杀死其他两个进程，SUSv3规定：

* 调用进程在执行命令期间应该忽略SIGINT和SIGQUIT信号
* 子进程对这两个信号的处理，如同调用fork和exec一样，即对所有已设信号的处置重置为SIG_DFL，而其他信号的处置则保持不变

改进版

---

注意事项：

* 针对”如果cmdstring为空，仅当system命令可用时，返回非0值，否则返回0“，唯一可靠的办法是递归调用system去运行shell命令，并检查返回状态
* 只有system的调用者才需要阻塞SIGCHLD，同时忽略SIGINT和SIGQUIT，必须在fork之前执行
* system的调用者必须使用waitpid，如果使用wait，可能捕获到其他子进程的状态

