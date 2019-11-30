### 第20章 信号：基本概念

##### 概念和概述

信号是事件发生时对进程的通知机制，也称为软件中断，进程可以向自己或其他进程发送信号，引发内核为进程产生信号的事件如下：

* 硬件发生异常：如被0除或引用了无法访问的内存区域
* 用户键入了能产生信号的特殊字符：中断字符（Control+C）、暂停字符（Control+Z）
* 软件事件：定时器到，某个子进程退出等

信号分为两类：内核向进程通知事件，构成所谓的传统或者标准信号、实时信号，在信号产生和到达期间，处于等待（pending）状态，有时需要确保一段代码不被信号打断，可以将信号添加到进程的信号掩码中以阻塞信号，稍后可解除，信号到达后，进程视具体信号执行如下默认动作之一：

1. 忽略信号：内核将其丢弃，进程不知道曾经出现过此信号
2. 终止进程：异常终止，而非调用exit的正常终止
3. 产生核心转储文件并终止进程
4. 停止进程：暂停进程的执行
5. 于之前暂停后再恢复进程的执行

##### 信号类型和默认行为

Linux标准信号编号是1-31，实际却超出，为了与其他UNIX实现兼容，其他则并未使用

* SIGALRM：alarm或settimer设置的定时器到期
* SIGBUS：总线错误表示发生了某种内存访问错误
* SIGCHILD：某一子进程终止，内核向父进程发生此信号，子进程因收到信号而停止或恢复时，也可能向父进程发生此信号
* SIGCLD：同上
* SIGCONT：将该信号发送给已经停止的进程，进程将恢复运行，当接收信号的进程当前不是停止状态，more忽略此信号
* SIGEMT：标识一个依赖于实现的硬件错误
* SIGFPE：特定类型的算术错误产生，如除0
* SIGHUP：终端断开（挂机）时，发送此信号给终端控制进程
* SIGILL：试图执行非法的机器语言指令
* SIGINFO：Linux中，同SIGPWR
* SIGINT：终端键入中断字符（Control+C），终端驱动程序发送此信号给前台进程组
* SIGIO：Linux中，同SIGABRT
* SIGKILL：处理器程序无法将其阻塞、忽略或捕获，总能终止进程
* SIGLOST：Linux中，未使用
* SIGPIPE：试图向管道、FIFO、套接字写入信息时，如果设备并没有相应的读进程，系统将产生此信号
* SIGPOLL：Linux中，同SIGIO
* SIGPROF：setitimer调用所设置的性能分析器（记录进程使用的CPU时间）一过期，内核就产生此信号
* SIGPWR：电源故障
* SIGQUIT：终端键入退出字符（Control+\），信号发给前台进程组，默认终止进程并产生核心转储文件
* SIGSEGV：引用无效内存
* SIGSTKFLT：Linux中，未使用
* SIGSTOP：处理器程序无法将其阻塞、忽略或捕获，总能停止进程
* SIGSYS：如果进程发起的系统调用有误就产生此信号
* SIGTERM：用来终止进程的标准信号，也是kill和killall命令发送的默认信号，用户有时候会用kill -KILL或kill -9显式的发送SIGKILL信号，通常是错误的，精心设计的程序应当为SIGTERM信号设置处理器程序，以便于预先清除临时文件和释放资源，发送SIGKILL信号可以杀掉进程，但是绕开了SIGTERM的信号处理程序，总是应该首先尝试SIGERM终止进程，SIGKILL是最后手段，去对付那些不响应SIGTERM信号的失控进程
* SIGTRAP：断点调试功能和strace命令
* SIGSTP：作业控制的停止信号，键盘输入挂起字符（Control+Z），发送此信号给前台进程组
* SIGTTIN：作业控制shell下后台进程组试图对终端进行read时
* SIGTTOU：类似SIGTTIN，针对的是后台作业的终端输出
* SIGUNUSED：顾名思义，未使用
* SIGURG：套接字上存在带外（紧急）数据
* SIGUSR1：供程序员使用，内核绝不会产生此信号
* SIGUSR2：同上
* SIGVTALRM：setitimer调用所设置的虚拟定时器（记录进程用户态使用的CPU时间）一过期，内核就产生此信号
* SIGWINCH：窗口环境中，终端窗口尺寸大小改变时
* SIGXCPU：进程的CPU时间超出对应的资源限制时
* SIGXFSZ：进程试图增大文件突破对进程文件大小的资源限制时

![WechatIMG29.jpeg](https://i.loli.net/2019/11/30/X573jq2OCE6orGb.jpg)

![WechatIMG30.jpeg](https://i.loli.net/2019/11/30/gK84VBfirdEwIHQ.jpg)

term表示终止进程，core表示产生核心转储文件，ignore表示忽略此信号，stop表示停止进程，cont表示信号恢复了一个已停止的进程

##### 改变信号处置：signal

signal的行为在不同UNIX实现有差别，sigaction应该是建立信号处理函数的首选API，在Linux中，signal是基于sigaction实现的glibc库函数

```
#include <signal.h>
void (signal(int signo, void (*handler)(int)))(int);
// 若成功，返回以前的信号处理配置，若出错，返回SIG_ERR
// handler可以指定为SIG_DFL(默认值),SIG_IGN(忽略)

typedef void Sigfunc(int);
Sigfunc *signal(int, Sigfunc *);
```

```
void (*old_handler)(int);
// switch to new
if (SIG_ERR == (old_handler = signal(SIGINT, new_handler)))
    perror("signal error");

/** do somthing with new_handler */

// retrieve to old
if (SIG_ERR == (signal(SIGINT, old_handler)))
    perror("signal error");
```

使用signal，无法在不改变信号处置的同时，还能获取当前的信号处置，即调用signal就会改变信号处置，sigaction可以做到这点

##### 信号处理器简介

调用信号处理器程序，可能随时打断主程序流程，内核代表进程调用处理器程序，当处理器返回时，主程序会在处理器打断的位置恢复执行，虽然处理器程序几乎可以为所欲为，但其设计应该力求简单

##### 发送信号：kill

之所以选择kill做术语，因为早期UNIX实现中大多数信号的默认行为是终止进程，kill将信号发送给进程或进程组，raise则允许向自身发送信号

```
#include <signal.h>

int kill(pid_t pid, int signo);
int raise(int signo);
// 两个函数，若成功，返回0，若出错，返回-1

raise(signo);
等价于
kill(getpid(), signo);
```

- pid>0：将信号发送给进程ID为pid的进程
- pid==0：将信号发送给与发送进程属于同一进程组的所有进程，且发送进程具有权限向这些进程发送信号
- pid<-1：将信号发送给进程组ID等于pid绝对值，且发送进程具有权限向这些进程发送信号
- pid==-1：将信号发送给发送进程具有权限向它们发送信号的所有进程，除去init和调用进程自身

进程要发送信号给另一个进程，需要一定权限，规则如下：

* 特权进程可以向任何进程发送信号
* 以root用户和组运行的init进程，仅能接收已经装了处理函数的信号
* 如果发送者的实际或有效用户ID匹配于接收者的实际用户ID或保存设置用户ID，非特权进程也可以向另一进程发送信号
* SIGCONT信号而言，无论对用户ID检查如何，非特权进程可以向同一会话中的任何其他进程发送这一信号

