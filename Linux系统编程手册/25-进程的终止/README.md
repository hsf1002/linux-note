### 第25章 进程的终止

##### 进程的终止：_exit和exit

```
#include <unistd.h>

void _exit(int status);
// status仅有低8位为父进程所用，0表示支持退出，非0表示异常退出，此调用总是会成功终止
```

程序一般不会直接调用 _exit，而是调用库函数exit，它在调用 _exit之前执行各种动作：

```
#include <stdlib.h>

void exit(int status);
// 执行的动作：
1. 调用退出处理程序（atexit和on_exit注册的函数）
2. 刷新stdio流缓冲区
3. 使用status提供的值执行_exit系统调用
```

return n等同于exit(n)的调用，因为main的返回值会作为exit的参数，如为显式执行return，C99标准要求，其等同于调用exit(0)

##### 进程终止的细节

无论进程是否正常终止，都会进行如下动作：

* 关闭所有打开文件描述符、目录流、信息目录描述符、（字符集）转换描述符
* 释放进程持有的任何文件锁
* 分离任何连接的System V共享内存段，对应于各段的shm_nattch计数减一
* 进程为每个System V信号量设置的semadj值加到信号量值中
* 如果是管理终端的进程，系统会向终端前台进程组每个进程发送SIGHUP信号，接着终端与会话脱离
* 关闭进程打开的任何POSIX有名信号量，类似调用sem_close
* 关闭进程打开的任何POSIX消息队列，类似于mq_close
* 如果某进程组成为孤儿，且该组存在任何已经停止进程，则组中所有进程都将收到SIGHUP信号，随之为SIGCONT信号
* 移除进程通过mlock或mlockall建立的任何内存锁
* 取消进程通过mmap创建的任何内存映射

##### 退出处理程序

可在进程生命周期任意点注册并在调用exit正常终止时自动执行，如是直接调用 _exit或因信号异常终止，则不会执行；应该为信号建立信号处理程序，并设置标记，主程序根据此标记调用exit（不是异步信号安全，不能在信号处理函数中调用），但是无法处理SIGKILL信号，因为无法改变其默认行为，这也是应该避免使用其终止进程的一个原因，建议使用SIGTERM，这是kill命令默认发送的信号

```
#include <stdlib.h>

int atexit(void (*fun)(void));
// 返回值：若成功，返回0，若出错，返回非0
// 可以注册的个数可以通过sysconf(_SC_ATEXIT_MAX)获取，但是无法获知有多少个已经注册的程序
// 通过fork创建的子进程会继承父进程注册的退出处理函数
// 无法取消由atexit或on_exit注册的退出程序，可以加全局标志来屏蔽此程序
```

atexit由两个限制：无法获知传递给exit的状态、无法给退出程序指定参数，glibc提供了一个非标准的替代方法：

```
#define _BSD_SOURCE
#include <stdlib.h>

int on_exit(void (*fun)(int, void*), void *arg);
// 使用atexit和on_exit注册的函数位于同一函数列表，可以同时使用两种方式
// 虽然更为灵活，但要保证移植性，应避免使用on_exit
```

##### fork、stdio缓冲区以及_exit之间的交互

在混合使用stdio函数和系统调用对同一文件IO处理时，需要特别谨慎：

* fork之前应该强制使用fflush刷新stdio缓冲区，或使用setbuf关闭stdio缓冲区
* 调用 _exit 而非exit，以避免刷新stdio缓冲区，更为通用的原则是父进程通过exit终止，其他子进程应调用 _exit终止，从而确保只有一个进程调用退出程序并刷新stdio缓冲区