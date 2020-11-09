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

### 从utmp和wtmp文件中检索信息

将utmp文件的当前位置设置到起始：

```
#include <utmpx.h>

void setutxent(void);
```

读取一条记录：

```
struct utmpx *getutxent(void);  // 顺序读取utmp文件的下一条记录
struct utmpx *getutxid(const struct utmpx *ut);   // 从当前位置读取与ut匹配的记录，根据ut参数中的ut_type和ut_id字段搜索
struct utmpx *getutxline(const struct utmpx *ut); // 从当前位置读取与ut匹配的记录，搜索字段ut_type为LOGIN_PROCESS或USER_PROCESS且ut_line与ut参数匹配的记录
// 若成功，返回一个指向utmpx结构的指针（静态分配的），若到文件尾或没有匹配的记录，返回NULL
```

关闭文件：

```
void endutxent(void);
```

指定文件：

```
#define _GNU_SOURCE
#include <utmpx.h>

int utmpxname(const char *file);
// 若成功，返回0，若出错，返回-1
// 默认情况下，搜索的是utmp文件，如果是另一个文件如wtmp，必须调用此函数
```

### 获取登录名称

```
char *getlogin(void);
// 若成功，返回登录名称，若出错，返回NULL
```

getlogin会调用ttyname找出与调用进程的标准输入关联的终端名，接着搜索utmp文件找出ut_line值与终端名匹配的记录，如果找到了，就返回ut_user字段；失败可能是进程没有一个与其标准输入关联的终端，或者进程本身是一个daemon，或者终端会话没有记录在utmp中

### 为登录会话更新utmp和wtmp文件

在创建一个登录会话的程序如login或sshd，应该遵循下面的步骤更新utmp和wtmp文件：

* 在登录的时候向utmp文件写入一条记录表明这个用户登录进系统了
* 在登出的时候应该删除之前写入的utmp文件的记录，如果因为程序奔溃而没有及时清理，下一次重启init会自动清理那些记录

通常utmp和wtmp文件只有特权用户才能更新

将ut指向的记录写入到utmp文件中：

```
struct utmp *pututxline(const struct utmpx *ut);
// 若成功，返回更新后的记录，若出错，返回NULL
// pututxline会首先使用getutxid搜索一个可被重写的记录，如果找到就重写，否则就追加
```

将ut指向的记录追加到wtmp文件中：

```
#define _GNU_SOURCE
#include <utmpx.h>

void updwtmpx(char *wtmpx_file, struct utmpx *ut);
// 将ut指向的记录追加到wtmpx_file的文件尾
```

### lastlog文件

lastlog记录着每个用户最近一次登录到系统的时间，提供登录服务的程序出了更新utmp和wtmp外，还应该更新lastlog文件，Linux上，此文件时/var/log/lastlog，通常也受保护

```
#define UT_NAMESIZE  32
#define UT_HOSTSIZE  256

struct lastlog
{
  time_t ll_time;
  char   ll_line[UT_NAMESIZE];
  char   ll_host[UT_HOSTSIZE];
}
```

并没有包含用户名或用户ID，lastlog文件中的记录用用户ID作为索引，如要找出用户ID为1000的lastlog的记录，需要到文件的位置（1000*sizeof(struct lastlog)）查找