## 第40章 登录记账

### utmp和wtmp概述

utmp：维护着当前登录进系统的用户记录，记录中包含ut_user字段，用户登出时删除这条记录，如who

wtmp：包含所有用户登录和登出的记录，登录时在写入utmp的同时写入wtmp相同记录，登出出再写入wtmp一条记录，如last

在Linux中，utmp文件位于/var/run/utmp，wtmp文件位于/var/log/wtmp，程序中应使用`_PATH_UTMP`和`_PATH_WTMP`

### utmpx API

Linux提供了传统的utmp和从System V演化而来的utmpx API，这两组API返回的信息是完全一样的，差别之一是utmp API中的一些函数时可重入的，而utmpx中的函数时不可重入的

### utmpx结构

utmp和wtmp文件中包含utmpx记录，在<utmpx.h>中定义：

```
struct utmp 
{
  short   ut_type;              /* Type of record */
  pid_t   ut_pid;               /* PID of login process */
  char    ut_line[UT_LINESIZE]; /* Device name of tty - "/dev/" */
  char    ut_id[4];             /* Terminal name suffix, or inittab(5) ID */
  char    ut_user[UT_NAMESIZE]; /* Username */
  char    ut_host[UT_HOSTSIZE]; /* Hostname for remote login, or kernel version for run-level messages */
  struct  exit_status ut_exit;  /* Exit status of a process marked as DEAD_PROCESS; not used by Linux init(8) */

  long   ut_session;           /* Session ID */
  struct timeval ut_tv;        /* Time entry was made */

  int32_t ut_addr_v6[4];        /* Internet address of remote host; IPv4 address uses just ut_addr_v6[0] */
  char __unused[20];            /* Reserved for future use */
};

ut_type的取值：
EMPTY：这个记录不包含有效的记录信息
RUN_LVL：系统启动或关闭时系统运行级别发生了变化
BOOT_TIME：这个记录包含ut_tv字段的系统启动时间
NEW_TIME：这个记录包含系统时钟变更之后的新时间
OLD_TIME：这个记录包含系统时钟变更之前的旧时间
INIT_PROCESS：记录由init进程孵化的进程
LOGIN_PROCESS：记录用户登录会话组长进程
USER_PROCESS：记录用户进程，通常是登录会话
DEAD_PROCESS：记录标识出已经退出的进程
```

