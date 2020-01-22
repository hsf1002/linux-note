## 第34章 进程组、会话和作业控制

### 概述

进程组：由一个或多个共享进程组ID的进程组成

进程组ID：首进程的ID，新进程会继承其父进程所属的进程组ID

进程组的生命周期：从首进程创建开始，到最后一个进程退出组（终止或加入另一个进程组）的时刻

会话：是一组进程组的集合

会话ID：首进程的进程ID，新进程会继承父进程的会话ID

一个会话中的所有进程共享一个控制终端，其会在会话首进程首次打开一个终端设备时被建立，会话首进程会成为该终端的控制进程，一个终端最多成为一个会话的控制终端；任意时刻，会话中只有一个进程组会成为终端的前台进程组，其他则是后台进程组，只有前台进程组才能从控制终端读取输入

SIGINT中断：Control+C、SIGQUIT退出：Control+\、SIGSTP挂起：Control+Z

会话和进程组的主要用途是用于shell作业控制，窗口环境下，控制终端是一个伪终端，每个终端窗口都是一个独立的会话，窗口的启动shell是会话首进程和终端的控制进程

显示当前shell的PID：echo $$

![WechatIMG37.jpeg](https://i.loli.net/2020/01/22/aAWeXzmcVF6Bnyh.jpg)

### 进程组

获取进程组ID：

```
#include <unistd.h>

pid_t getpgrp(void);
// 总是返回成功
// 如果返回值是调用进程的PID，说明调用进程即进程组的首进程
```

将pid所属的进程组ID修改为pgid：

```
#include <unistd.h>

int setpgid(pid_t pid, pid_t pgid);
// 若成功，返回0，若出错，返回-1
// 如果pid和pgid指定了同一个进程（即pgid是0或者它与pid的进程组ID匹配），就创建一个新进程组，且指定的进程是该进程组的首进程
// 如果pid和pgid指定的不是同一个进程（即pgid不是0或者它与pid的进程组ID不匹配），会将pid的进程移到pgid指定的进程组中

限制：
1. pid可以指定为调用进程或其中一个子进程
2. 在组之间移动进程，调用进程和由pid指定的进程以及目标进程组必须同属于一个会话
3. pid指定的进程不能是会话首进程
4. 一个进程在其子进程执行exec后就无法修改子进程的进程组ID了
```

编写作业控制shell程序时需要让父进程和子进程在fork之后立即调用setpgid将子进程的进程组ID设置为同样的值，且父进程需要忽略在setpgid调用中出现的所有EACCES错误

```
pid_t getpgid(pid_t pid);
pid_t getpgrp(void);                 /* POSIX.1 version */
pid_t getpgrp(pid_t pid);            /* BSD version */

int setpgrp(void);                   /* System V version */
int setpgrp(pid_t pid, pid_t pgid);  /* BSD version */
```

