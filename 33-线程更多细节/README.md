## 线程：更多细节

### 线程限制

可以通过sysconf函数查看线程相关限制

```
PTHREAD_DESTRUCTOR_ITERATIONS: 销毁一个线程数据最大的尝试次数，通过_SC_THREAD_DESTRUCTOR_ITERATIONS查询
PTHREAD_KEYS_MAX: 一个进程可以创建的最大key的数量,通过_SC_THREAD_KEYS_MAX查询 
PTHREAD_STACK_MIN: 线程可以使用的最小的栈空间大小, 通过_SC_THREAD_STACK_MIN查询
PTHREAD_THREADS_MAX:一个进程可以创建的最大的线程数, 通过_SC_THREAD_THREADS_MAX查询
```

### 线程属性

```
int pthread_attr_init(pthread_attr_t *attr);   
int pthread_attr_destroy(pthread_attr_t *attr); 
// 两个函数返回值：若成功，返回0，若出错，返回错误编号
```

pthread_attr_init会对属性对象动态分配空间，而pthread_attr_destroy不仅会释放该空间，还会用无效值初始化属性对象，如果该属性对象被误用，导致pthread_create函数返回错误码

线程属性包括：

```
detachstate: 线程的分离状态属性
guardsize: 线程栈末尾的警戒缓冲区大小
statckaddr: 线程栈的最低地址
stacksize: 线程栈的大小
--分离状态属性--
int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate); 
int pthread_attr_getdetachstate(pthread_attr_t *attr, int *detachstate);
// 两个函数返回值：若成功，返回0，若出错，返回错误编号
// detachstate可以取值PTHREAD_CREATE_DETACHED 以分离状态启动线程，或PTHREAD_CREATE_JOINABLE 正常启动，应用程序可以获取线程的终止状态
// 如果对线程终止状态不感兴趣，可以用pthread_detach设置让操作系统在线程退出时收回它所占用的资源
--线程栈缓冲区大小--
int pthread_attr_setstack(pthread_attr_t *attr,void *stackaddr, size_t stacksize);
int pthread_attr_getstack(pthread_attr_t *attr,void **stackaddr, size_t *stacksize);
// 两个函数返回值：若成功，返回0，若出错，返回错误编号
// 对进程而言，虚地址空间大小固定，而这个空间被所有线程共享，如果线程数量太多，就会减小默认的线程栈大小；或者线程的函数分配了大量的自动变量或涉及很深的栈帧，则需要的栈大小可能要比默认的大
// 如果线程栈的虚地址空间用完了，可以使用malloc或mmap作为可替代的栈分配空间，并用pthread_attr_setstack改变新线程栈的栈位置
// stackaddr为线程栈的最低内存地址，但不一定是栈的开始位置，如果CPU结构是从高地址往低地址增长，stackaddr将是栈的结尾位置，而不是开始位置
--线程栈的最低地址--
int pthread_attr_setguardsize(pthread_attr_t *attr, size_t guardsize);
int pthread_attr_getguardsize(pthread_attr_t *attr, size_t *guardsize);
// 两个函数返回值：若成功，返回0，若出错，返回错误编号
// guardsize控制着线程栈末尾之后用以避免栈溢出的扩展内存大小
// guardsize设置为0，不会提供警戒缓冲区；如果修改了stackaddr，希望认为我们自己管理栈，进而使警戒缓冲区无效，相当于guardsize设置为0
--线程栈的大小--
int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize);
int pthread_attr_getstacksize(pthread_attr_t *attr, size_t *stacksize);
// 两个函数返回值：若成功，返回0，若出错，返回错误编号
// 如果希望改变默认线程栈大小，而不想自己处理线程栈的分配问题，可以使用pthread_attr_setstacksize
// 设置stacksize时，不能小于PTHREAD_STACK_MIN
// Linux/x86-32架构上，除了主线程外的所有线程，栈大小默认值为2MB
```

### 线程与信号

##### UNIX信号模型如何映射到线程中

* 信号动作、信号处置，属于进程层面
* 信号的发送既可以针对进程，也可以针对线程，满足下面三条件之一就是面向线程
  * 信号源于硬件异常如SIGBUS、SIGFPE、SIGILL、SIGSEGV
  * 当线程试图对已断开的管道操作时产生的SIGPIPE
  * 由函数pthread_kill、pthread_sigqueue所发出的信号

* 当多线程程序收到一个信号，且进程已经为信号创建了处理程序，内核会任选一条线程来处理这个信号
* 信号掩码针对每个线程而言
* 备选信号栈为每个线程特有，新建线程并不继承
* 针对为每个进程挂起的信号，以及为每条线程所挂起的信号，内核分别维护有记录，sigpending会返回整个进程和当前线程挂起信号的并集
* 如果信号处理程序中断了pthread_mutex_lock的调用，该调用总是会自动重新开始，如果中断了pthread_cond_wait，该调用要么自动重新开始（Linux如此），要么返回0，表示遭遇了假唤醒

##### 操作线程信号掩码

把线程引入编程范型，使得信号的处理变得更加复杂。单个线程可以阻止某些信号，当某个线程修改了与某个给定信号相关的处理行为后，所有的线程都必须共享这个处理行为的改变。如一个线程忽略某个信号，则另一个线程就可以通过两种方式撤销上述线程的信号选择：恢复信号的默认处理行为，或为信号设置一个新的信号处理程序。如果一个信号与硬件故障有关，则该信号一般会发生到引起该事件的线程，其他信号则被发送到任意一个线程。进程中使用sigprocmask阻止信号发送，线程中则使用pthread_sigmask

```
int pthread_sigmask(int how, const sigset_t *restrict set, sigset_t *restrict oset);
// 若成功，返回0，若出错，返回错误编号
```

- 工作方式与sigprocmask基本相同，how的取值
  - SIG_BLOCK：把信号集添加到线程信号屏蔽字中
  - SIG_SETMASK：用信号集替换线程的信号屏蔽字
  - SIG_UNBLOCK：从线程信号屏蔽字中移除信号集
- 如果oset不为空，则获取线程的信号屏蔽字并保存到oset
- 如果set不为空，则设置线程的信号屏蔽字为set，如果set为空，oset不为空，则how被忽略

##### 向线程发送信号

发送信号给进程，可以调用kill，发送信号给线程，可以调用pthread_kill

```
int pthread_kill(pthread_t thread, int signo);
// 若成功，返回0，若出错，返回错误编号
// 无法发送信号给其他进程的线程
```

可以传0给signo检查线程是否存在，如果信号的默认处理动作是终止该进程，那么把信号传递给某个线程仍然会杀死整个进程

Linux特有函数pthread_sigqueue将pthread_kill和sigqueue的功能合二为一：

```
#include <signal.h>
#include <pthread.h>

int pthread_sigqueue(pthread_t thread, int sig, const union sigval value);
// 如成功，返回0，若出错，返回正数
```

##### 妥善的处理异步信号

可以通过调用sigwait等待一个或多个信号的出现

```
int sigwait(const sigset_t *set, int *signop);
// 若成功，返回0，若出错，返回错误编号
```

- set指定了等待的信号集
- signop指向的整数将包含发送信号的数量
- 如果信号集中某个信号在调用sigwait时处于挂起状态，那么sigwait无阻塞的返回，返回之前，从进程中移除那些处于挂起的信号；为了避免错误行为发生，线程在调用sigwait前，必须阻塞那些它正在等待的信号；sigwait会原子的取消信号集的阻塞状态，直到新的信号被递送，返回之前，sigwait将恢复线程的信号屏蔽字

sigwait和sigwaitinfo几乎相同，除了以下差异：

* sigwait只返回信号编号，而非返回一个siginfo_t的结构
* 返回值与其他线程相关函数一致，而非传统的系统调用返回0或-1

多个线程调用sigwait等待同一信号，只有线程会实际接收，但无法确定是哪个

多线程程序必须要处理异步信号时，推荐的方法：

* 所有线程都阻塞进程可能接收的所有异步信号：在创建任何其他线程之前，由主线程阻塞，后续创建的每个线程都会继承主线程的信号掩码
* 再创建一个专有线程，调用函数sigwaitinfo、sigtimedwait或sigwait来接收信号

### 线程和进程控制

##### 线程和exec

除了调用exec的线程之外，其他所有线程立刻消失。没有任何线程会针对线程特有数据执行解构函数、也不会调用清理函数。该进程的所有互斥量和属于进程的条件变量都会消失。调用exec后，调用线程的线程ID是不确定的

##### 线程与fork

多线程进程调用fork，仅会将发起调用的线程复制到子进程中，其他线程在子进程都会消失，也不会为这些线程执行针对线程特有数据执行解构函数、也不会调用清理函数。但是子进程继承了整个地址空间的副本，包括每个互斥量、读写锁和条件变量。多线程中调用fork推荐的方式：其后紧随exec的调用；对于那些必须执行fork，其后又无exec跟随的程序而言，可以通过pthread_atfork建立fork处理程序，最多可以安装三个帮助清理锁状态的函数

```
int pthread_atfork(void (*prepare)(void), void (*parent)(void),void (*child)(void));
// 若成功，返回0，若出错，返回错误编号
```

- prepare：在fork创建子进程前调用，任务是获取父进程定义的所有锁
- parent：在fork创建子进程后，返回之前在父进程的上下文中调用，任务是对prepare获取的所有锁进行解锁
- child：在fork返回之前，在子进程上下文中调用，任务是释放prepare获取的所有锁

##### 线程与exit

如果任何线程调用了exit，或主线程执行了return，那么所有线程都会消失，也不会执行线程特有数据的解构函数以及清理函数

### 线程实现模型

三种模型的差异体现在线程如何与内核调度实体（KSE，Kernel Scheduling Entity）相映射

##### 多对一实现：用户级线程

线程创建、调度、同步、互斥量的锁定、条件变量的等待，所有细节全部在用户空间处理，内核对于进程中的多个线程一无所知，因为无需切换到内核模式，创建速度很快，劣势：

* 如果read遭到阻塞，所有其他线程都会阻塞
* 内核无法对这些线程进行调度

##### 一对一实现：内核级线程

每个线程对应一个单独的KSE，线程创建、上下文切换、同步等操作要慢一些，尽管有这些缺点，通常更优于M:1模式，LinuxThreads和NPTL都采用1：1模型

##### 多对多实现：两级模型

每个进程都拥有多个与之相关的KSE，也可以把多个线程映射到一个KSE，允许内核将同一应用的线程调度到不同的CPU上运行，同时也解决了随线程数量而放大的性能问题，M:N模型最大问题是过于复杂，线程调度任务由内核及用户空间的线程库共同承担，二者需要进行分工协作和信息交换

### Linux POSIX线程的实现

针对Ptheads API：Linux有两种实现

* LinuxThreads：最初的线程实现（已经过时，glibc从2.4版本开始不再支持）
* NPTL（Native POSIX Threads Library）：更符合SUSv3的标准，性能优于LinuxThreads

##### LinuxThreads

实现要点：

* 线程的创建使用clone，指定标志：CLONE_VM|CLONE_FILES|CLONE_FS|CLONE_SIGHAND
* 会额外创建一个附加的管理线程，负责处理其他线程的创建和终止
* 利用信号处理内部操作

对于标准行为的背离之处：

* 同一进程的不同线程调用getpid返回不同值
* 只有创建子进程的线程才可以使用wait
* 线程之间不会共享凭证
* 线程不共享一般的任务号和进程组号
* 不共享使用fcntl建立的记录锁
* 不共享资源限制
* 不共享nice值

##### NPTL

NPTL弥补了LinuxThreads的大部分缺陷，其实现的测试程序可以创建10万个线程，而LinuxThreads实际的线程数量限制大约是2000个

创建线程使用clone，指定标志：CLONE_VM|CLONE_FILES|CLONE_FS|CLONE_SIGHAND|CLONE_THREAD|CLONE_SETTLS|CLONE_PARENT_SETTID|CLONE_CHILD_CLEARTID|CLONE_SYSVSEM

##### 确认线程实现

`getconf GNU_LIBPTHREAD_VERSION`

