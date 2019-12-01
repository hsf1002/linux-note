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

##### 发送信号的其他方式：raise和killpg

```
#include <signal.h>

int raise(int signo);
// 返回值：若成功，返回0，若出错，返回非0值，唯一可能出错的地方是signo无效而返回EINVAL

单线程中:
raise(signo)相当于kill(getpid(), signo);
多线程中：
raise(signo)相当于pthread_kill(pthread_self(), signo);
```

```
#include <signal.h>

int killgg(pid_t pgrp, int signo);
// 返回值：若成功，返回0，若出错，返回-1

killpg(pgrp, signo)相当于kill(-pgrp, signo)
如果指定pgrp为0，那么会向调用者所属进程组的所有进程发送此信号
```

##### 检查进程的存在

kill的signo指定为0，可以发送空信号，以此可以检查目标进程是否存在，若空信号发送失败，且errno为ESRCH，表明目标进程不存在，如果调用成功或调用失败且errno为EPERM，表示目标进程存在；但是验证一个特定进程是否存在并不能保证其正在运行，此外，可能存在但却是僵尸（其父进程尚未执行wait来获取其终止状态），可以通过其他途径检查进程是否正在运行：

* wait系统调用
* 信号量和排它锁：如果进程持续持有某个信号量或排它锁，且一直处于被监控的状态，那么如能获取到信号量或锁时，表明该进程已经终止
* 管道和FIFO：对监控目标进程进行设置，令其在自身生命周期内持有对通道进行写操作的打开文件描述符，而监控进程则持有对通道进行读操作的打开文件描述符，且通道写入端关闭，即可获知监控目标进程已终止
* /proc/PID接口：如/proc/PID/12345，对其进行stat检查

##### 显示信号描述

每个信号与之相关的描述位于数组sys_siglist中，如sys_siglist[SIGPIPE]获取SIGPIPE信号（管道断开）的描述，相对比直接使用数组，更推荐调用strsignal函数：

```
#define _BSD_SOURCE
#include <signal.h>

extern const char *const sys_siglist[];

#define _GNU_SOURCE
#include <string.h>

char *strsignal(int signo);
// 返回信号对应的描述字符串，如果signo无效，返回错误字符串
// 相较于sys_siglist，strsignal会进行边界检查，而且对本地（local）设置敏感

void psignal(int signo, char *msg);
// msg后加冒号，再显示信号描述
```

##### 信号集

数据类型是sigset_t

```
#include <signal.h>

int sigemptyset(sigset_t *set);
// 将信号集初始化为set指向的信号集，清除所有信号

int sigfillset(sigset_t *set);
// 将信号集初始化为set指向的信号集，包含所有信号
// sigfillset或sigemptyset必须只能执行一次

int sigaddset(sigset_t *set, int signo);
// 把信号signo添加到信号集set中

int sigdelset(sigset_t *set, int signo);
// 把信号signo从信号集set中删除

这四个函数，若成功，返回0，若出错，返回-1

int sigismember(sigset_t *set, int signo);
// 如果是返回1，如果不是，返回0，如果给定的信号无效，返回-1；
```

GNU C实现了3个非标准函数，是对上述信号集标准函数的补充：

```
#define _GNU_SOURCE
#include <signal.h>

int sigandset(sigset_t *dest, sigset_t *left, sigset_t *right);
// 将left和right的交集置于dest
int sigorset(sigset_t *dest, sigset_t *left, sigset_t *right);
// 将left和right的并集置于dest
int sigisemptyset(const sigset_t *set);
// 若set集未包含信号，则返回true
```

##### 信号掩码（阻塞信号传递）

内核会为每个进程维护一个信号掩码，即一组信号，将阻塞其针对该进程的传递，信号掩码属于线程属性，多线程中，每个线程都可以使用pthread_sigmask独立检查和修改其信号掩码；如果将阻塞的信号发送给某进程，那么对该信号的传递将延后，直到从进程掩码中移除该信号，从而解除阻塞为止

```
#include <signal.h>

int sigpromask(int how, const sigset_t *restrict set, sigset_t *restrict oset);
// 返回值：若成功，返回0，若出错，返回-1
```

- 若oset是非空指针，那么进程的当前信号屏蔽字通过oset返回

- 若set是非空指针，那么参数how指示如何修改当前信号屏蔽字

  ```
  SIG_BLOCK：将set指向的包含了希望阻塞的信号集，与当前信号屏蔽字，相并，或操作
  SIG_UNBLOCK：将set指向的包含了希望阻塞的信号集的补集，与当前信号屏蔽字，相交，与操作
  SIG_SETMASK：将当前的信号集合设置为set指向的信号集，赋值操作
  ```

- 如果set是空指针，那么不改变进程的信号屏蔽字，how无意义

如果解除了对某个信号的锁定，那么会立刻将该信号传递给进程，系统将忽略试图阻塞SIGKILL和SIGSTOP信号的请求，如果试图阻塞，sigprocmask既不会处理，也不会产生错误，这意味着，可以使用如下方式阻塞除了SIGKILL和SIGSTOP之外的所有信号：

```
// 使用blockset(包含所有信号)初始化信号集
sigfillset(&blockset);
// 将所有信号阻塞，但实际无法阻塞SIGKILL和SIGSTOP
if (-1 == sigprocmask(SIG_BLOCK, &blockset, NULL))
    perror("sigprocmask error");
```

##### 处于等待状态（pending）的信号

返回的信号集由参数set返回，对于调用进程而言，其中的各个信号是阻塞不能传递的，因而一定是当前未决的；如果某进程接受了一个该进程正在阻塞的信号，那么会将该信号添加到进程的等待信号集中，当解除了对该信号的锁定时，随之将信号传递给此进程

```
#include <signal.h>

int sigpending(sigset_t *set);
// 返回值：若成功，返回0，若出错，返回-1
```

##### 不对信号进行排队处理

如果同一信号在阻塞状态下产生多次，那么会将该信号记录在等待信号集中，稍后仅传递一次；即使进程没有阻塞信号，其收到的信号可能比发送给它的要少得多，如果信号发送速度如此之快，以至于内核考虑将执行权调度给接收进程前，这些信号已经到达，就会发生这种情况

##### 改变信号处置：sigaction

sigaction较之于signal，允许在获取信号处置的同时无需将其改变，还可以设置各种属性对调用信号处理程序时的行为控制的更加精确，可移植性也更加：

```
#include <signal.h>

int sigaction(int signo, conststruct sigaction*restrict act, struct sigaction*restrict oact);
// 若成功，返回0，若出错，返回-1
// signo是除去SIGKILL和SIGSTOP之外的任何信号

struct sigaction{
  void (*sa_handler)(int);
  sigset_t sa_mask;
  int sa_flag;
  void (*sa_sigaction)(int, siginfo_t*, void*);
};
// sa_handler对应于signal的handler参数，是信号处理函数的地址，或者是常量SIG_IGN、SIG_DFL之一
// 仅当sa_handler是信号处理函数的地址，即SIG_IGN、SIG_DFL之外的取值，才会对sa_mask和sa_flag加以处理
// sa_sigaction和sa_handler，在应用中只能一次使用其中之一

sa_flag的选项：
SA_INTERRUPT: 由此信号中断的系统调用不自动重启动
SA_NOCLDSTOP: 若signo是SIGCHLD，当子进程停止，不产生此信号，当子进程终止，仍旧产生此信号，若已设置此标志，当停止的进程继续运行时，不产生SIGCHLD信号
SA_NOCLDWAIT:若signo是SIGCHLD，当调用进程的子进程终止时，不创建僵死进程，当调用进程随后调用wait，则阻塞到它所有子进程都终止
SA_NODEFER: 当捕捉到此信号执行其信号处理函数时，系统不自动阻塞此信号，应用于早期不可靠信号
SA_ONSTACK: XSI
SA_RESETHAND: ...
SA_RESTART: 由此信号中断的系统调用自动重启动
SA_SIGINFO: 对信号处理程序提供了附加信息：一个指向siginfo的指针以及指向上下文的context指针
```

- signo是要检测或修改的信号编号
- 若act非空，则修改其动作，若oact非空，则系统经由oact返回该信号的上一个动作

一般信号处理程序调用：

```
void handler(int signo);
```

如果设置sa_flag为SA_SIGINFO，则调用：

```
void handler(int signo, siginfo_t *info, void *context)
```

siginfo包含了信号产生原因有关信息：

```
typedef struct __siginfo {
	int	si_signo;		/* signal number */
	int	si_errno;		/* errno association */
	int	si_code;		/* signal code */
	pid_t	si_pid;			/* sending process */
	uid_t	si_uid;			/* sender's ruid */
	int	si_status;		/* exit value */
	void	*si_addr;		/* faulting instruction */
	union sigval si_value;		/* signal value */
	long	si_band;		/* band event for SIGPOLL */
	unsigned long	__pad[7];	/* Reserved for Future Use */
} siginfo_t;

union sigval {
	/* Members as suggested by Annex C of POSIX 1003.1b. */
	int	sival_int;
	void	*sival_ptr;
};
```

传递信号时，在si_value.sival_int传递一个整型或si_value.sival_ptr传递一个指针，SIGCHLD包含的si_code：

```
#define	CLD_EXITED	1	/* [XSI] child has exited */
#define	CLD_KILLED	2	/* [XSI] terminated abnormally, no core file */
#define	CLD_DUMPED	3	/* [XSI] terminated abnormally, core file */
#define	CLD_TRAPPED	4	/* [XSI] traced child has trapped */
#define	CLD_STOPPED	5	/* [XSI] child has stopped */
#define	CLD_CONTINUED	6	/* [XSI] stopped child has continued */
```

若信号是SIGCHLD，则设置si_pid, si_status和si_uid字段，若信号时SIGBUS、SIGILL、SIGFPE或SIGSEGV，则si_addr包含造成故障的根源地址，该地址可能并不准确

context是无类型参数，可被强制转为ucontext_t结构类型，用于标识信号传递时进程上下文

##### 等待信号：pause

将暂停进程的执行，直到捕捉一个信号，即信号处理器函数中断该调用为止（或一个未处理信号终止进程为止）

```
#include <unistd.h>

int pause(void);
// 返回值：-1， errno设置为EINTR
```

只有执行了一个信号处理函数并从其返回时，pause才返回

