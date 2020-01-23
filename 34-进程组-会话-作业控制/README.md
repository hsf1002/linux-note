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

### 会话

获取会话ID：

```
#define _XOPEN_SOURCE 500
#include <unistd.h>

pid_t getsid(pid_t pid);
// 若成功，返回pid指定的进程所属的会话ID，若出错，返回-1
// 如果pid指定为0，返回调用进程的会话ID
```

如果调用进程不是进程组首进程，则创建一个新会话：

```
#include <unistd.h>

pid_t setsid(void);
// 若成功，返回新会话的ID，若出错，返回-1

1. 调用进程会成为新会话的首进程和会话中新进程组的首进程
2. 调用进程没有控制终端，所有之前到控制终端的连接都会断开
```

如果调用进程是一个进程组首进程，那么setsid调用报EPERM错误，避免的方式是执行fork并让父进程直至以及让子进程调用setsid，由于子进程继承父进程的进程组ID并接收属于自己的唯一的进程ID，因为它无法成为进程组首进程

### 控制终端和控制进程

控制终端会被由fork创建的子进程继承并在exec调用时得到保持，当会话首进程打开一个控制终端后它同时成为了该终端的控制进程，如果一个进程拥有一个控制终端，打开/dev/tty就能获取该终端的文件描述符

使用ioctl(fd, TIOCNOTTY)可以删除进程与文件描述符指定的控制终端之间的关联，如果调用进程是控制终端的控制进程，控制进程终止时：

1. 会话中所有进程会失去与控制终端之间的关联
2. 因此另一个会话首进程就能够获取该终端以成为控制进程
3. 内核会向前台进程组的所有成员发送SIGHUP信号

获取控制终端的路径名：

```
#include <stdio.h>

char *ctermid(char *ttyname);
// 通过返回值或参数返回控制终端的路径名
```

### 前台和后台进程组

在一个会话中，同一时刻，只有一个进程组是前台进程组，只有一个进程是前台进程

获取一个终端的进程组：

```
#include <unistd.h>

pid_t tcgetpgrp(int fd);
// 若成功，返回fd指定的终端的前台进程的进程组ID，若出错，返回-1
```

将终端的前台进程组ID修改为pgid：

```
int tcsetpgrp(int fd, pid_t pgid);
// 若成功，返回0，若出错，返回-1
// pgid必须与调用进程所属的会话中一个进程的进程组ID匹配
```

### SIGHUP信号

发送SIGHUP的条件：

* 终端驱动器检测到连接断开
* 终端窗口被关闭

SIGHUP的默认动作是终止进程

向控制进程发送SIGHUP信号会引起链式反应，从而导致将SIGHUP发送给很多其他进程，两种情况：

##### 在shell中处理SIGHUP信号

登录会话中，shell通常是控制进程，大多数shell程序在交互运行时会为SIGHUP建立处理器，其会终止shell，但在终止之前会向shell创建的各个进程组（前台进程组和后台进程组）发送SIGHUP信号，但是不会向不是由该shell创建的进程组发送此信号

##### SIGHUP和控制进程的终止

Linux上，SIGHUP后会跟随一个SIGCONT信号以确保之前被停止的进程组可以恢复

### 作业控制

1980年BSD系统的C shell推出的特性，它允许一个shell用户同时执行多个命令（作业），其中一个命令在前台运行，其他在后台运行，作业可以被停止和恢复，以及在前后台之间移动

##### 在shell中使用作业控制

输入的命令以&结束，该命令作为后台任务运行，作业号显示在方括号内：

```
sleep 600 &
[2] 20222
sleep 500 &
[3] 20223
```

列出所有后台作业：

```
jobs
[2]-  Running                 sleep 600 &
[3]+  Running                 sleep 500 &
```

将后台作业移动到前台：

```
fg %2
sleep 600
```

将前台作业挂起（使用Control+Z，会向前台进程组发送SIGTSTP信号）：

```
^Z
[2]+  Stopped                 sleep 600
```

在后台恢复挂起的任务（shell会发送任务一个SIGCONT信号）：

```
jobs
[2]+  Stopped                 sleep 600
[3]-  Running                 sleep 500 &
bg %2
[2]+ sleep 600 &
jobs
[2]-  Running                 sleep 600 &
[3]+  Running                 sleep 500 &
```

停止后台作业（发送SIGSTOP）：

```
jobs
[2]-  Running                 sleep 600 &
[3]+  Running                 sleep 500 &

kill -STOP %2
[2]+  Stopped                 sleep 600

jobs
[2]+  Stopped                 sleep 600
[3]-  Running                 sleep 500 &
```

只有前台进程才能从控制终端读取输入，如果后台作业尝试读取输入，会接收到一个SIGTTIN信号，其默认动作是终止进程；默认情况下，后台作业是被允许向控制终端输入内容，如果终端设置了TOSTOP标记（终端输出停止），当后台作业尝试在终端输出时导致SIGTTOU信号，其默认动作也是终止进程

![WechatIMG38.jpeg](https://i.loli.net/2020/01/22/DKvNVwyXWjR8eHB.jpg)

#####  实现作业控制

SIGCONT信号：内核允许一个进程如shell向同一会话中的任意进程发送SIGCONT信号，不管是否有权限

SIGTTIN和SIGTTOU信号：

* 当进程当前处于阻塞状态或忽视SIGTTIN信号的状态，则不发送SIGTTIN信号，这时如果试图从控制终端发起read调用会失败并返回EIO
* 如果终端设置了TOSTOP标记，当进程当前处于阻塞状态或忽视SIGTTIN信号的状态，则不发送SIGTTOU信号，这时控制终端发起write调用是允许的（即TOSTOP标记被忽视了）
* 不管是否设置了TOSTOP标记，后台进程试图在控制终端试图调用会修改终端驱动器数据结构的特定函数（如tcsetpgrp、tcsetattr、tcflush、tcflow、tcsendbreak、tcdrain）时会生成SIGTTOU信号

##### 处理作业控制信号

处理SIGTSTP信号的细节问题：如果它被捕获，就不会执行默认的终止进程的动作，可以让其处理器生成一个SIGSTOP信号，但这种方式不够准确，恰当的处理方式是让SIGTSTP信号再生成一个SIGTSTP信号来停止进程：

1. 处理器将SIGTSTP信号的处理重置为默认SIG_DFL
2. 处理器生成SIGTSTP信号
3. 由于SIGTSTP信号会被阻塞进入处理器（除非指定SA_NODEFER），因此处理器会解除该信号的阻塞，这时生成的SIGTSTP信号会导致默认动作的执行，立即挂起进程
4. 当进程接收到SIGCONT信号时会恢复
5. 返回之前，处理器会重新阻塞SIGTSTP信号并重新注册本身来处理下一个SIGTSTP信号，重新阻塞SIGTSTP信号的目的是为了防止处理器重新注册本身之后和返回之前接收到另一个SIGTSTP信号导致处理器被递归调用的情况

对于作业控制信号（SIGTSTP、SIGTTIN、SIGTTOU），在非作业控制shell中，这些信号的处理被设置成了SIG_IGN，只有作业控制shell时才会设置为SIG_DFL，类似的规则适用于其他由终端产生的信号如：SIGINT、SIGQUIT、SIGHUP

##### 孤儿进程组

如果一个进程组至少有一个成员拥有一个位于同一会话但不同进程组的父进程，就不是孤儿进程组，由于shell没有创建子进程，它无法确定子进程是否存在以及子进程和已经退出的父进程是否位于同一进程组中，init进程只会检查被终止的子进程并清理该僵尸进程，从而导致被停止的子进程可能永远残留在系统；如果一个进程组变成了孤儿进程组且拥有很多已经停止的成员，系统会向进程组所有成员发送SIGHUP信号通话会话已经断开，接着发送一个SIGCONT信号确保恢复停止的的进程，如果孤儿进程组不包含被停止的成员，则不发送任何信号

一个进程组变成孤儿进程组的原因：

* 最后一个位于不同进程组但属于同一会话的父进程终止了
* 父进程位于的另一个进程组中的最后一个进程终止了

