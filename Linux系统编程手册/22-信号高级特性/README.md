### 第22章 信号：高级特性

##### 核心转储文件

特定信号会引发进程创建一个核心转储文件并终止运行，核心转储文件时内含进程终止时内存映像的文件（core是一种老迈的内存技术），通常是键入退出符（Control + \）而生成SIGQUIT信号，Linux下可以使用gdb连接一个正在运行的进程，使用gcore命令获取其core文件

不产生核心转储文件的情况：

* 进程对核心转储文件没有写权限（默认是进程当前工作目录）
* 存在一个同名、可写的普通文件，但指向该文件的（硬）链接数超过一个
* 将要创建的核心转储文件所在目录不存在
* 把进程核心转储文件大小的资源限制（RLIMIT_CORE）置为0，shell下可通过ulimit设置
* 将进程可创建文件的大小的资源限制（RLIMIT_FSIZE）置为0
* 对进程正在执行的二进制可执行文件没有读权限（防止借助核心转储文件获取程序代码）
* 以只读方式挂载当前工作目录的文件系统，或文件系统空间已满，又或者i-node资源耗尽
* set-user-ID（set-group-ID）程序在由非文件属主（或属组）执行时，不会产生核心转储文件（借助于Linux专有调用prctl的PR_SET_DUMPABLE操作，可为进程设置dumpable标志，当非文件属主运行set-user-ID（set-group-ID）程序时，即可产生核心转储文件）

Linux特有的/proc/PID/coredump_filter，可以对写入核心转储文件的内存映射类型施以进程级控制，有4种内存映射：私有匿名映射、私有文件映射、共享匿名映射、共享文件映射，文件默认值提供了传统的Linux行为：仅对私有匿名映射和共享匿名映射进行转储

Linux特有的/proc/sys/kernel/core_pattern，可用格式化字符串为核心转储文件重命名，其默认名是core

```
%c  文件大小的资源软限制
%e  可执行文件名
%g  遭转储进程的实际组ID
%h  主机系统的名称
%p  遭转储进程的进程ID
%s  导致进程终止的信号编号
%t  转储时间
%u  遭转储进程的实际用户ID
%%  单个%字符
```

##### 传递、处置和处理的特殊情况

SIGKILL和SIGSTOP的默认行为是终止和停止一个进程，无法改变，如果试图使用signal或sigaction改变，总是返回错误，也不能阻塞这两个信号，这意味着总是可以使用它们终止或停止一个失控进程

SIGCONT可以使某些（因接收SIGSTOP、SIGTSTP、SIGTTIN、SIGTTOU）处于停止状态的进程得以继续运行，即使该进程处于正在阻塞或忽略SIGCONT信号，如果处于停止的进程接收的是其他信号，在接收到SIGCONT恢复运行之前，信号实际上并未传递，SIGKILL属于例外

如果程序在执行时发现，已经将对由终端产生信号的处置设置为SIG_IGN（忽略），程序通常不应该试图去改变信号处置

##### 可中断和不可中断的进程睡眠状态

SIGKILL和SIGSTOP信号对进程的作用是立竿见影的，但是有限制：

* TASK_INTERRUPTIBLE：进程正在等待某一事件如等待终端输入、等待数据写入管道等，为这种状态下的进程产生信号，那么操作中断，传递来的信号将唤醒进程，用ps命令查看处于此状态的进程STAT字段标记为S
* TASK_UNINTERRUPIBLE：进程正在等待某些特定类型的事件如磁盘IO的完成，为这种状态下的进程产生信号，那么在进程结束这种状态之前，系统不会把信号传递给进程，用ps命令查看此状态进程的STAT字段标记为D
* TASK_KILLABLE：类似于TASK_UNINTERRUPIBLE，但是会在进程收到一个致命信号时将其唤醒

##### 硬件产生的信号

硬件异常可以产生SIGBUS、SIGFPE、SIGILL、SIGSEGV，调用kill也可以发送这些信号，但是很少见，正确处理硬件产生的信号方法有二：

1. 接受信号的默认行为（如终止进程）
2. 为其编写不会正常返回的处理函数如_exit或siglongjmp，确保将控制传递回主程序的某一位置

因为在硬件异常情况下，如果进程从此类信号的处理函数返回，或进程忽略或阻塞此类信号，进程的行为未定义

##### 信号的同步生成和异步生成

异步：引发信号产生的事件，其发生与进程的执行无关

同步：硬件异常导致的5种信号或进程通过raise、kill或killpg向自身发送信号

##### 信号传递的时间与顺序

同步产生的信号立即传递，异步产生的信号即使没有阻塞，在信号产生与实际传递之间可能存在一个瞬时延迟，在此期间，信号处于等待状态，内核将等待信号传递给进程的时机是：该进程正在运行，且发生由内核态到用户态的下一次切换时，这意味着在如下时刻才会传递信号：

1. 进程再次获得调度时
2. 系统调用完成时（信号的传递可能引起正在阻塞的系统调用过早完成）

如果进程使用sigprocmask解除对多个等待信号的阻塞，所有信号会立刻传递该该进程，顺序是信号的编号按升序以此传递，与信号产生的次序无关；当多个等待信号解除阻塞时，而在信号处理函数执行期间发生了内核态和用户态的切换，那么将中断此处理器函数的执行，转而去调用第二个信号处理函数

##### signal的实现及可移植性

signal在不同实现中具有不同的语义，特别是早期的实现并不可靠，这意味着：

* 刚一进入信号处理函数，会将信号处置重置为其默认行为
* 在信号处理函数执行期间，不会对新产生的信号进行阻塞

鉴于此，sigaction是建立信号处理函数的首选

##### 实时信号

较之于标准信号的优势：

1. 信号范围有所扩大，标准信号中可供随意使用的仅有SIGUSR1和SIGURS2
2. 队列化管理，同样的实时信号发送多次，将传递多次，而标准信号只传递一次
3. 可以指定伴随数据（一整型或指针）
4. 多个实时信号处于等待状态，率先传递最小编号的那个，如果是同一类型，与发送信号的顺序一致

RTSIG_MAX：实时信号的可用数量

SIGRTMIN：实时信号编号的最小值

SIGRTMAX：实时信号编号的最大值

为了可移植性，实时信号编号的定义应该使用SIGRTMIN + x的形式

RLIMIT_SIGPENDING：限制了可排对的信号总数

/proc/PID/status的SigQ字段：正在等待某一进程的实时信号数量

使用实时信号的规则：

1. 发送进程使用sigqueue系统调用发送信号及其伴随数据，kill、killpg、raise也可以发送，但不能保证排序
2. 要为该信号建立信号处理函数，接收进程应以SA_SIGINFO标志发起对sigacton的调用
3. 在sigaction结构的sa_sigacton而不是通常的sa_handler提供信号处理程序

发送实时信号：

```
#include <signal.h>

int sigqueue(pid_t pid, int signo, const union sigval value);
// 若成功，返回0，若出错，返回-1，一旦超过排队限制，将调用失败，置errno为EAGAIN
// 只能把信号发送给单个进程，可以使用value传递整型或指针，发送的信号不能被无限排队，最大为SIGQUEUE_MAX

union
{
    int sival_int;   // 伴随数据之整型
    void *sival_ptr; // 伴随数据之指针，很少使用
}
```

处理实时信号：

```
struct sigaction act;

sigemptyset(&act.sa_mask);
act.sa_sigaction = handler;
act.sa_flags = SA_RESTART | SA_SIGINFO;

if (sigaction(SIGRTMIN + 5, &act, NULL) == -1)
    perror("sigaction error");
```

一旦使用SA_SIGINFO标志，信号处理函数的第二个参数必须是一个siginfo_t结构，包含实时信号的附加信息：

```
typedef struct __siginfo 
{
	int	si_signo;		/* 信号编号 */
	int	si_errno;		/* 错误码 */
	int	si_code;		/* 信号来源的深入信息 */
	pid_t	si_pid;			/* 发送进程的PID */
	uid_t	si_uid;			/* 发送进程的真实用户ID */
	int	si_status;		/* 子进程的退出状态 */
	void	*si_addr;		/* 针对硬件产生的SIGBUS和SIGSEGV表示引发无效内存的地址，对于SIGILL和SIGFPE而言，表示信号产生的程序指令地址 */
	union sigval si_value;		/* 伴随数据 */
	long	si_band;		/* IO事件相关的“带事件“值 */
	unsigned long	__pad[7];	/* Reserved for Future Use */
} siginfo_t;
```

##### 使用掩码来等待信号：sigsuspend

场景：

1. 临时阻塞一个信号，防止其信号处理函数不会将某些关键代码片段中断
2. 解除对此信号的阻塞，暂停执行，直到有信号到达

要达到此目的，需要将解除信号阻塞和挂起进程两个动作封装为一个原子操作：

```
#include <signal.h>

int sigsuspend(const sigset_t *sigmask);
// 总是返回-1，且将errno设置为EINTR

// 1. 进程的信号屏蔽字设置为sigmask
// 2. 在捕捉到一个信号或发生了一个会终止该进程的信号之前，该进程被挂起
// 3. 如果捕捉到一个信号且从该信号处理函数返回，则sigsuspend返回，且该进程的信号屏蔽字设置为调用sigsuspend之前的值
```

相当于以不可中断方式执行如下操作：

```
sigprocmask(SIG_SETMASK, &block_mask, &prev_mask) // assign new mask
pause();
sigprocmask(SIG_SETMASK, &prev_mask, NULL)  // restore old mask
```

主要用途：

* 保护代码临界区，使其不被特定信号中断

- 等待一个信号处理程序设置一个全局变量
- 实现父进程、子进程之间的同步

##### 以同步方式等待信号

作为sigsuspend的替代方案，使用更为简单：

```
#include <signal.h>

int sigwaitinfo(const sigset_t *set, siginfo_t *info);
// 返回值：若成功，返回发送的信号个数，若出错，返回-1
// 调用会挂起进程，直至set信号集中某个信号到达，如果该信号处于等待状态，则立即返回
// info若不为空，包含信号与信号处理函数中的参数相同
// 不对标准信号排序，仅对实时信号排序，且遵循低编号优先
// 调用sigwaitinfo而不阻塞set中的信号将导致不可预知的行为
```

```
int sigtimedwait(const sigset_t *set, siginfo_t *info, const struct timespec *timeout);
// 返回值：若成功，返回发送的信号个数，若出错或超时，返回-1
// 如果将timeout的两个字段都指定为0，则立刻超时返回，如果将timeout指定为NULL，则等同于sigwaitinfo
```

##### 通过文件描述符来获取信号

signalfd是Linux提供的非标准系统调用，可以创建一个特殊的文件描述符，发送给进程的信号都可以从此读取，为同步接收信号提供了sigwaitinfo之外的另一种选择：

```
#include <sys/signalfd.h>

int signalfd(int fd, const sigset *mask, int flags);
// 返回值：若成功，返回文件描述符，若出错，返回-1
// 如同sigwaitinfo一样，通常也应该调用sigprocmask阻塞mask中的信号，确保有机会读取这些信号前，不会按默认处置
// 如果指定fd为-1，会创建一个新的文件描述符，用于读取mask中的信号，否则将修改与fd相关的mask值
// flag可以指定为SFD_CLOEXEC或SFD_NONBLOCK
```

##### 利用信号进行进程间通信

相比较于其他IPC，信号编程既繁且难，原有如下：

* 信号的异步本质意味着要面对各种问题，如可重入需求、竞争条件、信号处理函数中处理全局变量（如果用sigwaitinfo或signalfd来同步信号，这些问题大部分不会遇到）
* 没有对标准信号进行排序，对于实时信号，存在排队数量限制
* 信号所携带的信号量有限，一个字节，过低的带宽使得信号传输极为缓慢

##### 早期的信号API（System V和BSD）

System V信号API：

```
#define _XOPEN_SOURCE 500

#include <signal.h>

void (*sigset(int signo, void (*handler)(int)))(int);
// 返回值：若成功，返回上一次的信号处置，如果信号被阻塞，返回SIG_HOLD，若出错，返回-1
// handler的参数可以是SIG_IGN、SIG_DFL或信号处理函数的地址，或指定为SIG_HOLD，将信号添加到信号屏蔽字而保持信号处置不变

int sighold(int signo);  // 添加
int sigrelse(int signo); // 移除
int sigignore(int signo);// 设定某一信号的处置为忽略
// 返回值：若成功，返回0，若出错，返回-1

int sigpause(ing signo); // 类似sigsuspend
// 总是返回-1，同时置errno为EINTR
```

BSD信号API：

```
#define _BSD_SOURCE
#include <signal.h>

int sigvec(int signo, struct sigvec *vec, struct sigvec *ovec); // 类似sigaction
// 返回值：若成功，返回0，若出错，返回-1

int sigblcok(int mask);  // 类似sigprocmask的SIG_BLOCK
int sigsetmask(int mask);// 类似sigprocmask的SIG_SETMASK
// 返回值：都返回上个信号屏蔽字

int sigpause(int sigmask);
// 总是返回-1，同时置errno为EINTR

int sigmask(int sig);   // 类似sigsuspend，与System V具有不同的调用签名
// 将信号编号转换为相应的32位掩码值
```

