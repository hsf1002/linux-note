### 第26章 监控子进程

##### 等待子进程

当一个进程正常或异常终止时，内核就向父进程发送SIGCHLD信号，系统的默认动作时忽略它，如果调用wait或waitpid则：

- 如果其所有子进程都在运行，则阻塞
- 如果一个子进程已经终止，正等待父进程获取其终止状态，则取得该子进程的终止状态立即返回
- 如果没有任何子进程，则出错返回-1并将errno置为ECHILD

```
#include <sys/wait.h>

pid_t wait(int *status); 
// 返回值，若成功，返回进程ID，若出错，返回0或-1

如下代码可等待所有子进程退出：
while ((child_pid = wait(NULL)) != -1)
    continue;
if (errno != ECHILD)
    perror("wait error");
另一种退出循环的方法是记录终止子进程的数量当与创建子进程数量相同时，退出循环
如果同一时间多个子进程退出，其顺序取决于具体实现，Linux各个版本也不尽相同
```

waitpid并不等待第一个终止子进程，它有若干选项，可以控制所等待的子进程，提供了三个wait没有的功能

- waitpid可等待一个特定的进程，而wait则返回任一终止子进程的状态
- waitpid提供了wait的非阻塞版本，有时候希望获取子进程的状态，而不阻塞
- wait只能发现已经终止的子进程，对于因信号（SIGSTOP或SIGTTIN）而停止，而后恢复（SIGCONT）的情况则无能为力
- waitpid通过WUNTRACED和WCONTINUED选项支持作业控制

```
#include <sys/wait.h>

pid_t waitpid(pid_t pid, int *status, int options);
// 返回值，若成功，返回进程ID，若出错，返回0或-1
```

- status如果为空，表示不关心终止状态，如果不为空，则保存终止状态
- waitpid的第一个参数说明
  - pid=-1：等待任一子进程，等同于wait
  - pid=>0：等待进程ID与pid相等的子进程
  - pid==0：等待组ID等于调用进程组ID的任一子进程
  - pid<-1：等待组ID等于pid绝对值的任一子进程
- option参数说明：
  * WNOHANG：	若由pid指定的子进程未发生状态改变(没有结束)，则waitpid()不阻塞，立即返回0
  * WUNTRACED： 除了返回终止子进程的信号外，还返回因信号停止的子进程信息
  * WCONTINUED：返回收到SIGCONT信号而恢复执行的已停止子进程状态信息

wait和waitpid返回的status值，可以区分如下子进程事件：

1. 子进程调用exit或 _exit终止，并指定一个整型值作为退出状态
2. 子进程收到未处理信号而终止
3. 子进程因为信号而停止，并以WUNTRAED标志调用waitpid
4. 子进程因收到信号SIGCONT恢复，并以WCONTINUED标志调用waitpid

以下四个互斥的宏可以取得子进程终止的原因：

```
WIFEXITED(status)：子进程正常结束则为真，可以通过WEXITSTATUS(status)取得子进程exit()返回的结束代码
WIFSIGNALED(status)：异常终止子进程则为真，可以通过WTERMSIG(status)取得子进程因信号而中止的信号代码
WIFSTOPPED(status)：子进程处于暂停状态则为真，可以通过WSTOPSIG(status)取得引发子进程暂停的信号代码
WIFCONTINUED(status)：在作业控制暂停后已经继续的子进程返回则为真
```

默认情况下某些信号会终止进程，如果希望在终止之前做些清理工作，需要设置信号处理程序来捕获信号，如果在信号处理程序中调用 _exit(EXIT_SUCCESS)，父进程会认为子进程是正常终止，如果需要通知父进程自己因某个信号而终止，那么子进程的信号处理程序应首先将自己废除，再次发出相同信号，该信号这次将终止子进程如：

```
void
handler(int signo)
{
    // 首先废除自己
    signal(signo, SIGDFL);
    // 再次发送信号
    raise(sino);
}
```

waitid类似于waitpid，但提供了更多的灵活性，控制更为精准：

```
#include <sys/wait.h>

pid_t waitid(idtype_t idtype, it_t id, siginfo_t, *infop, int options);
// 返回值：若成功，返回0，若出错，返回-1
```

使用两个单独的参数表示要等待的子进程所属的类型，id的作用和idtype有关，idtype的类型：

* P_PID：等待一个特定的进程：id等于等待子进程的进程ID
* P_PGID：等待一个特定进程组中的任一子进程：id等于要等待子进程的进程组ID
* P_ALL：等待任一子进程：忽略id

option的状态：

* WEXITED：等待已退出的进程，无论是否正常返回
* WNOHANG：与waitpid语义相同，如无可用的子进程退出状态，立即返回而非阻塞
* WNOWAIT：不破坏子进程退出状态。该子进程退出状态可由后续的wait、waitid或waitpid调用取得
* WSTOPPED：等待一个通过信号而停止的进程

- WCONTINUED：等待一个进程，它以前曾被停止，此后又已继续

有个细节，如果option是WNOHANG，那么waitid返回0意味着两种情况：在调用时子进程的状态已经改变或者没有任何子进程的状态有所改变：

```
siginfo_t info;
...
memset(&info, 0x00, sizeof(siginfo_t));

if (-1 == waitid(idtype, id, &info, option | WNOHANG))
    perror("waitid error");
    
// 任何子进程的状态都未改变
if (0 == info.si_pid)
    ;
// 一个子进程的状态已经改变
else 
    ;
```

wait3和wait4提供的功能比wait、waitpid、waitid要多一个，与附加参数有关，该参数允许内核返回由终止进程及所有子进程使用的资源概括

```
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
 
pid_t wait3(int *status, int options,
            struct rusage *rusage);
pid_t wait4(pid_t pid, int *status, int options,
            struct rusage *rusage);
// 两个函数的返回值：若成功，返回进程ID，若出错，返回-1
wait3等同于waitpid(-1, &status, option)，等待的是任意子进程
wait4等同于waitpid(pid, &status, option)，等待的是一个或多个子进程
```

返回的资源信息包括：用户CPU时间总量、系统CPU时间总量、缺页次数、接收到信号的次数等

```
struct rusage 
{
    struct timeval ru_utime;
    struct timeval ru_stime;
    long   ru_maxrss;       
    long   ru_ixrss;        
    long   ru_idrss;        
    long   ru_isrss;        
    long   ru_minflt;       
    long   ru_majflt;       
    long   ru_nswap;        
    long   ru_inblock;      

    long   ru_oublock;      
    long   ru_msgsnd;       
    long   ru_msgrcv;       
    long   ru_nsignals;     
    long   ru_nvcsw;        
    long   ru_nivcsw;       
};
```

##### 孤儿进程与僵尸进程

如果某一进程的父进程终止后，对getppid的调用将返回1，这是判定某一进程之“生父”是否“在世”的方法之一，前提是假设该进程由init之外的进程创建；如果父进程在执行wait之前子进程就已经终止，内核通过将子进程转换为僵尸进程来处理，将释放子进程所持有的大部分资源，仅在内核进程表中保留一条记录，包含进程ID、终止状态、资源使用数据等信息，当父进程执行wait后，内核将删除僵尸进程，如果父进程未执行wait退出，那么init进程将接管子进程并自动调用wait，从而从系统中移除僵尸进程；如果存在大量僵尸进程，将填满进程表，从而阻碍新进程的创建，既然无法用信号杀死僵尸进程，唯一方法便是杀死他们的父进程（或等待其父进程终止），此时init接管和等待，然后清除它们；在设计长生命周期的父进程时应执行wait方法，确保系统总是能够清除那些死去的子进程，避免使其变成长寿僵尸

##### SIGCHLD信号

子进程的终止属于异步事件，父进程无法预知其何时终止，应使用wait或类似调用防止僵尸子进程的累积，以及如下两种方法避免这一个问题：

1. 父进程调用不带WNOHANG标志的wait或waitpid，如果没有以及终止的子进程，调用将阻塞
2. 父进程周期性的调用带有WNOHANG标志的waitpid，执行对已终止子进程的非阻塞式检查

这两种方法使用起来都不方便，为了规避这些问题，可以采用针对SIGCHLD信号的处理程序，无论一个子进程何时终止，系统都会向父进程发送SIGCHLD信号，对该信号的默认处理是忽略，也可以安装信号处理函数捕获它，当调用信号处理程序时，会暂时引发调用信号阻塞起来（除非为sigaction指定了SA_NODEFER标志），且不会对SIGCHLD等标准信号进行排队，当SIGCHLD信号处理程序运行时，有两个子进程终止，即产生了两次SIGCHLD信号，父进程也只能捕获一个，结果是父进程的SIGCHLD信号处理程序只调用一次wait，那么一些僵尸子进程可能成为漏网之鱼，解决方案是在SIGCHLD处理程序内部循环以WNOHANG标志来调用waitpid，直到没有其他终止的子进程需要处理为止

建议在创建任何子进程之前就设置好SIGCHLD信号处理程序，考虑到可重入性问题，系统调用waitpid等可能改变errno，因此在SIGCHLD信号处理程序中需要保存并恢复errno的值

向已停止的子进程发送SIGCHLD信号：

如果未使用SA_NOCLDSTOP标志，系统会在子进程停止时向父进程发送SIGCHLD信号，如果使用了此标志，那么子进程停止时就不会向父进程发送SIGCHLD信号，因为默认会忽略SIGCHLD，因此SA_NOCLDSTOP标志仅仅在设置SIGCHLD信号处理程序才有意义，当信号SIGCONT导致已停止的子进程恢复执行时，也向父进程发送SIGCHLD信号

忽略终止的子进程：

更有可能这样处理终止子进程：将对SIGCHLD的处置显示设置为SIG_IGN，系统从而会将终止的子进程立刻删除，不会转化为僵尸进程，会将返回状态弃之不问，后续的wait调用不会返回子进程的任何信息；虽然对信号SIGCHLD的默认处置就是忽略，但是显示设置会导致行为差异，Linux中这样设置并不会影响既有僵尸进程的状态，唯一完全可移植的办法是（可能是从SIGCHLD信号处理程序内部）调用wait或者waitpid

SUSv3规定：如果将对SIGCHLD的处置设置为SIG_IGN，同时父进程已终止的子进程并无处于僵尸状态被等待的情况，那么wait或waitpid调用将一直阻塞，直至所有子进程都终止，届时将返回错误ECHILD；SA_NOCLDWAIT标志的作用类似于对SIGCHLD设置SIG_IGN的效果，他们之间最主要的区别是：当以SA_NOCLDWAIT标志设置处理程序时，SUSv3并未规定系统在子进程终止时是否向其父进程发送SIGCHLD信号