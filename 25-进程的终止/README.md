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