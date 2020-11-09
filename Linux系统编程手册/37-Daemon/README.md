## 第37章 Daemon

### 概述

守护进程的特征：

* 生命周期很长，一般在系统启动时创建直到系统关闭
* 后台运行且不拥有控制终端，内核永远不会为守护进程生成任何任务控制信号和终端相关信号

常见的守护进程：

cron：规定时间内执行命令的程序

sshd：允许在远程主机使用一个安全的通信协议登录系统

httpd：HTTP的服务器程序

inetd：Internet超级服务器程序，监听从指定的TCP/IP端口上进入的网络连接

pdflush：定期将脏页面（即高速缓冲区的页面）写入磁盘

### 创建一个daemon

1. 执行一个fork，之后父进程退出，子进程继续执行，这样daemon会成为init的子进程
2. 子进程调用setsid，开启一个新会话并释放它与控制终端之间的所有关联
3. 如果daemon后续可能打开一个终端设备，需确保这个设备不会变成控制终端，有两个方法
   * open的调用中指定O_NOCTTY标记
   * setsid之后执行第二个fork，再次让父进程退出，并让子进程继续执行，这样子进程不会成为会话组长，从而不会重新请求一个控制终端

4. 清除进程的umask
5. 修改进程的当前工作目录，通常会更改为根目录/
6. 关闭daemon从父进程继承而来的所有打开的文件描述符
7. 关闭文件描述符0、1、2后，打开/dev/null（一个虚拟设备，总是将写入的数据丢弃），并使用dup2使得这三个描述符指向这个设备，原因：
   * 确保daemon调用了在这些描述符上执行IO的库函数时不会失败
   * 防止daemon后面使用1、2打开一个文件的情况

### 使用SIGHUP重新初始化一个daemon

daemon持续运行的障碍：

* daemon通常在启动时从配置文件读取参数，有时候需要在不重启的情况下快速修改这些参数
* 一些daemon会产生日志，如果永远不关闭日志，最终会阻塞文件系统

解决方案是让daemon为SIGHUP建立一个处理器，在收到信号时采取措施，内核不会发送此信号

### 使用syslog记录消息和错误

syslog从两个不同的源接收日志消息：UNIX domain socket的/dev/log，保存本地产生的消息；以及Internet domain socket，保存通过TCP/IP网络发送的消息

每条消息都包括：

facility：产生消息的程序类型

level：消息的严重程度

syslogd daemon会检查每条消息的facility和level，根据配置文件/etc/syslog.conf中的指令将消息传递到几个可能目的地中的一个，如终端、虚拟控制台、磁盘文件、FIFO

/dev/log的另一个消息来源是klogd，收集内核消息，通过printk产生

##### syslog API

建立一个到系统日志的连接，可选，如忽略后续的syslog调用默认设置：

```
#include <syslog.h>

void openlog(const char *ident, int log_options, int facility);
// ident是一个字符串，syslog的每条输出都会包含它，通常是程序名，如果是NULL，glibc syslog会将程序名赋值

log_option的取值：
LOG_CONS：向系统日志发送消息发生错误时将消息写入系统控制台/dev/console
LOG_NDELAY：立刻打开到日志系统的连接，默认是LOG_ODELAY，首次调用syslog时才打开，chroot时有用
LOG_ODELAY：与LOG_NDELAY相反
LOG_NOWAIT：Linux不起作用
LOG_PERROR：将消息写入标准错误和系统日志
LOG_PID：每条消息上加入调用者的进程ID

facility的取值：
LOG_AUTH：认证系统：login、su、getty等
LOG_AUTHPRIV：同LOG_AUTH，但只登录到所选择的单个用户可读的文件中
LOG_CRON：cron守护进程
LOG_DAEMON：其他系统守护进程，如routed
LOG_FTP：文件传输协议：ftpd、tftpd
LOG_KERN：内核产生的消息
LOG_LPR：系统打印机缓冲池：lpr、lpd
LOG_MAIL：电子邮件系统
LOG_NEWS：网络新闻系统
LOG_SYSLOG：由syslogd（8）产生的内部消息
LOG_USER：随机用户进程产生的消息
LOG_UUCP：UUCP子系统
LOG_LOCAL0~LOG_LOCAL7：为本地使用保留
```

记录一个日志消息：

```
void syslog(int priority, const char *format, ...);
// priority是facility和level的OR值
// facility的默认值是openlog指定的，如果那个调用省略了，则是LOG_USER
// level的取值：
LOG_EMERG：紧急情况 
LOG_ALERT：应该被立即改正的问题，如系统数据库破坏
LOG_CRIT：重要情况，如硬盘错误
LOG_ERR：常规错误
LOG_WARNING：警告信息
LOG_NOTICE：不是错误情况，但是可能需要处理
LOG_INFO：情报信息
LOG_DEBUG：包含情报的信息，通常旨在调试一个程序时使用
```

关闭日志：

```
void closelog(void);
// 释放了分配给/dev/log socket的文件描述符
```

过滤日志：

```
int setlogmask(int mask);
// 返回上次的mask，所有level不在当前掩码中的消息都被丢弃
```

/etc/syslog.conf控制着syslogd daemon的操作，规则如下：

```
facility.level    action

facility的取值：
auth    认证系统，即询问用户名和口令
cron    系统定时系统执行定时任务时发出的信息
daemon  某些系统的守护程序的syslog,如由in.ftpd产生的log
kern    内核的syslog信息
lpr     打印机的syslog信息
mail    邮件系统的syslog信息
mark    定时发送消息的时标程序
news    新闻系统的syslog信息
user    本地用户应用程序的syslog信息
uucp    uucp子系统的syslog信息
local0..7 种本地类型的syslog信息,这些信息可以又用户来定义
\*       代表以上各种设备

level的取值：
emerg   紧急，处于Panic状态。通常应广播到所有用户
alert   告警，当前状态必须立即进行纠正。例如，系统数据库崩溃 
crit    关键状态的警告。例如，硬件故障
err     其它错误 
warning 警告 
notice  注意；非错误状态的报告，但应特别处理 
info    通报信息
debug   调试程序时的信息
none    通常调试程序时用，指示带有none级别的类型产生的信息无需送出

action的取值：
/filename 日志文件，由绝对路径指出的文件名，此文件必须事先建立 
@host     远程主机，@符号后面可以是ip或域名，默认在/etc/hosts文件下loghost这个别名已经指定给了本机
user1,user2 指定用户，如果指定用户已登录，那么他们将收到信息
*         所有用户，所有已登录的用户都将收到信息

实例：
*.err        /dev/tty10  // level为err或更高信息发送到/dev/tty10
auth.notice  root        // level为notice或更高信息发送到root登录的所有控制台和终端
*.debug;mail.none;news.none  -/var/log/message // 多个选择器用封号隔开，将mail和news之外的所有消息发送到/var/log/message，-表示无需每次写入文件都将文件同步到磁盘
```

每次修改syslog.conf文件后都需要让daemon根据这个文件重新初始化自身：

```
killall -HUP syslogd
```

