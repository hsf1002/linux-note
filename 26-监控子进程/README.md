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

