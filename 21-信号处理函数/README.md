### 第21章 信号处理函数

##### 设计信号处理函数

一般情况下，信号处理函数设计的越简单越好，这将降低引发竞争条件的风险，两种常见设计：

* 信号处理函数设置全局标志变量并退出
* 信号处理函数执行某种类型的清理动作，接着终止进程或使用非本地跳转，将控制返回到主程序的预订位置

在处理器函数执行期间，如果多次产生同类信号，那么仍然会将会将其标记为等待状态，稍后只传递一次，在信号处理函数中，并非所有系统调用和库函数都可以安全调用；如果同一进程的多个线程可以同时安全的调用某一函数，那么该函数就是可重入的，安全意味着，无论其他线程调用该函数的执行状态如何，函数都能产生预期结果

可重入和非可重入函数：

1. 更新全局变量或静态数据结构的函数是不可重入的
2. malloc函数族等和使用他们的其他库函数是不可重入的
3. 使用经静态分配的内存返回信息的函数如crypt()、getpwnam()、gethostbyname()、getservbyname()等是不可重入的
4. 将静态数据结构用于内部记账的函数如stdio库成员printf()、scanf()等会为缓冲区更新内部结构，也是不可重入的

标准的异步信号安全函数：

即当信号处理函数调用时，其实现是安全的函数，如果某一函数是可重入的，或者信号处理函数无法将其中断时，该函数就是异步信号安全的

![WechatIMG31.jpeg](https://i.loli.net/2019/12/02/94BZKrQOamoqzx7.jpg)

基本规则是在信号处理函数中，绝对不要调用不安全的函数

全局变量和sig_atomic_t数据类型：

尽管存在可重入问题，有时候仍然需要在主程序和信号处理函数之间共享全局变量，一般的设计是信号处理函数修改全局变量，主程序周期性的检查这个标志，此变量应该总是声明为volatile，防止被编译器优化到寄存器中，而sig_atomic_t可以保证原子性

```
volatile sig_atomic_t flag;
```

##### 终止信号处理函数的其他方法

1. 使用_exit ，不要使用exit，因为不安全，它会刷新stdio缓冲区
2. 使用kill发送信号杀掉进程
3. 执行非本地跳转

如果使用longjmp函数从信号处理函数中退出存在一个问题：BSD中，进入信号处理函数时，内核自动将引发调用的信号以及由act.sa_mask指定的任意信号添加到进程的信号掩码中，并在处理函数正常返回时再将它们从掩码中删除；System V以及Linux中，退出信号处理函数时longjmp不会将信号掩码恢复（通常这并不是希望的行为），鉴于此，POSIX定义了两个新函数，针对执行非本地跳转时对信号掩码进行显示控制：

```
#include <setjmp.h>

int sigsetjmp(sigjmp_buf env, int savemask);
// 若直接调用则返回0，若从siglongjmp调用返回则返回非0值

void siglongjmp(sigjmp_buf env, int val);

// 若savemask非0，则sigsetjmp在env中保存进程的当前信号屏蔽字，调用siglongjmp从其中恢复保存的信号屏蔽字
// 若savemask是0，则不会保存和恢复进程的信号屏蔽字
```

4. 使用abort终止进程并产生核心转储文件

```
#include <stdlib.h>

void abort(void);
// 将SIGABRT信号发送给调用进程，进程不应忽略此信号，此信号的默认动作是终止进程并产生核心转储文件
// 无论忽略或阻塞SIGABRT信号，abort调用都不受影响，除非进程捕获此信号后信号处理函数尚未返回，否则必须终止进程
```

##### 在备选栈中处理信号：sigaltstack

在调用信号处理函数时，内核通常会在进程栈中为其创建一帧，如果栈的大小到了RLIMIT_STACK，内核将为该进程产生SIGSEGV信号，而栈空间已经耗尽，内核无法再为进程已经安装的SIGSEGV处理函数创建帧，处理函数将得不到调用，进程就终止了；可以利用sigaltstack创建一个备选信号栈：

```
#include <signal.h>

int sigaltstack(const stack_t *sigstack, stack_t *old_sigstack);
// 返回值：若成功，返回0，若出错，返回-1

typedef struct
{
    void *ss_sp;	// 备选栈的起始地址
    int ss_flags; // SS_ONSTACK, SS_DISABLE
    size_t ss_size; // 备选栈大小
}stack_t;
```

SIGSEGV处理函数的工作不是在执行清理工作后终止进程，就是使用非本地跳转解开标准栈，ss_flags的取值：

* SS_ONSTACK：进程正在备选栈上运行，此时调用sigaltstack来创建将会产生一个错误EPERM
* SS_DISABLE：在old_sigstack上返回，表示当前不存在已创建的备选信号栈，如果在sigstack指定，则会禁用当前以及创建的备选信号栈

##### SA_SIGINFO标志

如果在sigaction创建时设置了SA_SIGINFO标志，收到信号时处理函数可以获取该信号的一些附加信息，需要将处理器声明为：

```
void handler(int signo, siginfo_t *siginfo, void *context);
// context是无类型参数，可被强制转为ucontext_t结构类型，用于描述信号处理函数前的进程状态，包括一个进程信号掩码以及寄存器保存值，如CP和SP

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

union sigval {
	/* Members as suggested by Annex C of POSIX 1003.1b. */
	int	sival_int;
	void	*sival_ptr;
};

struct sigaction{
  union
  {
    void (*sa_handler)(int);
    void (*sa_sigaction)(int, siginfo_t*, void*);
  }
  sigset_t sa_mask;
  int sa_flag;
  void (*sa_restorer)(void) 
};
// sa_handler和sa_sigaction只能设置其一

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

##### 系统调用的中断和重启

系统调用阻塞如read时，如果进程接收到信号并调用信号处理函数，从信号处理返回，将导致系统调用失败，并将errno置为EINTR，可手动重启系统调用：

```
while ((cnt = read(fd, buf, BUF_SIZE)) == -1 && errno == EINTR)
  continue;

if (cnt == -1)
  perror("read error");
```

但这种处理方式需要对每个阻塞的系统调用添加代码，很麻烦，可以调用指定了SA_RESTART标志的sigaction来创建信号处理函数，让内核代表进程自动重启系统调用，无需处理系统调用可能返回的EINTR错误，该标志针对信号而言，即允许某些信号的处理函数中断阻塞的系统调用，而其他系统调用则可以自动重启

SA_RESTART标志对哪些系统调用（和库函数）有效：

linux中以下阻塞的系统调用（以及在此基础上构建的库函数）遭到中断时可以自动重启：

* 等待子进程的系统调用：wait、waitpid、wait3、wait4、waittid
* 访问慢速设备时的IO系统调用：read、readv、write、writev、ioctl
* 系统调用open
* 套接字的各种系统调用：accept、accept4、connect、send、sendmsg、sendto、recv、recvfrom、recvmsg，如果使用setsockopt设置超时，这些系统调用不会自动重启
* POSIX消息队列进行IO操作的系统调用：mq_receive、mq_timedreceive、mq_send、mq_timedsend
* 用于设置文件锁的系统调用：flock、fcntl、lockf
* Linux特有的系统调用futex的FUTEX_WAIT操作
* 用于递减POSIX信号量的sem_wait和sem_timedwait
* 用于同步POSIX线程的函数：pthread_mutex_lock、pthread_mutex_trylock、pthread_mutex_timedlock、pthread_cond_wait、pthread_cond_timedwait

linux中以下阻塞的系统调用（以及在此基础上构建的库函数）遭到中断时不会自动重启：

* poll、ppoll、select、pselect这些IO多路复用调用
* Linux特有的epoll_wait和epoll_pwait系统调用
* Linux特有的io_getevents系统调用
* 操作System V消息队列和信号的阻塞系统调用：semop、semtimedop、msgrcv、msgsnd
* 对inotify文件描述符发起的read调用
* 用于将进程挂起指定时间的系统调用和库函数：sleep、nanosleep、clock_nanosleep
* 特意设计用来等待某一信号到达的系统调用：pause、sigsuspend、sigtimedwait、sigwaitinfo

为信号修改SA_RESTART标志：

```
#include <signal.h>

int sigintertupt(int signo, int flag);
// 返回值：若成功，返回0，若出错，返回-1

// 若flag是真，针对信号signo处理函数将中断阻塞的系统调用，如是假，信号处理函数结束后自动重启阻塞的系统调用
// 该接口SUSv4已经废止，推荐使用sigaction
```

对于某些Linux系统调用，未处理的停止信号会产生EINTR错误：

Linux上，即使没有信号处理函数，某些阻塞的系统调用也会产生EINTR错误，如果系统调用遭到阻塞，并且进程因信号（SIGSTOP、SIGTSTP、SIGTTIN、SIGTTOU）而停止，之后又收到SIGCONT信号恢复执行时，就会发生这种错误，如下的系统调用和库函数具有这一行为：

* epoll_pwait、epoll_wait、对inotify的文件描述符执行read、semop、semtimedop、sigwaitinfo、sigtimedwait

这种行为的结果是程序因为信号而停止和重启，需要添加代码来重新启动这些系统调用，即使程序并未停止信号设置信号处理函数