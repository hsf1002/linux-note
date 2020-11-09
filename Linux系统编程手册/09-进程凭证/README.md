### 第9章 进程凭证

##### 概述

每个进程都有一套用数字表示的用户ID，组ID，有时候将这些ID称之为进程凭证：

* 实际用户ID（real user ID）和实际组ID（real group ID）
* 有效用户ID（effective user ID）和有效组ID（effective group ID）
* 保存的set-user-id（saved set-user-ID）和保存的set-group-id（saved set-group-ID）
* 文件系统用户ID（file-system user ID）和文件系统组ID（file-system group ID）
* 辅助组ID

##### 实际用户ID和实际组ID

确定了进程所属的用户和组

##### 有效用户ID和有效组ID

有效用户ID和组ID和相对于的实际ID相等，但有两种方法使其不同：系统调用或执行set-user-ID和set-group-ID程序

##### set-user-ID和set-group-ID程序

set-user-ID程序会将进程的有效用户ID设置为可执行文件的用户ID，从而获得常规情况下并不具备的权限，set-group-ID程序对进程有效组ID实现类似任务

```
ls -l prog
-rwxr-xr-x

chmod u+s prog	// 打开set-user-ID权限位
chmod g+s prog	// 打开set-grop-ID权限位
```

打开权限位之后，可执行权限x标识会被s标识替换

```
ls -l prog
-rwsr-sr-s
```

运行set-user-ID的程序时，内核会将有效用户ID设置为可执行文件的用户ID，set-group-ID程序对进程有效组ID的操作类似，通过这种方式修改进程的有效用户ID或组ID，能够使进程获取常规情况下不具备的权限；如果一个可执行文件的属主是root，为此程序设置了set-user-ID权限位，运行程序时，进程会取得超级用户权限；也可利用此机制，将进程的有效ID修改为root之外的其他用户，如要访问一个受保护文件，可以创建一个具有对该文件访问权限的专有用户（组）ID，再创建一个set-user-ID（set-group-ID）程序，将进程有效用户（组）ID变更为这个专用ID，这样无需拥有超级用户的所有权限，程序就能访问该文件

Linux系统中常用的set-user-ID程序包括passwd：修改用户密码，mount和unmount：加载卸载文件系统，su：允许用户以另一个身份运行shell

```
su
password:
chown root check_password // 修改程序所属为root
chmod u+s check_password  // 设置set-user-ID权限位有效
ls -al check_password
-rwsr-xr-x
exit
whoami			// 并非root用户
mtk
./check_password	// 却可以访问shadown文件
username:avr
password:
successfully authenticated: UID=1001
```

set-user-ID和set-group-ID技术集实用性和强大功能于一身，但一旦涉及欠佳可能造成安全隐患

##### 保存set-user-ID和保存set-group-ID

当执行程序时，依次发生如下事件：

1. 若可执行文件的set-user-ID权限位开启，则将进程的有效用户（组）ID设置为可执行文件的属主，若未开启此权限位，则进程的有效用户（组）ID保持不变
2. 保持set-user-ID和保存set-group-ID的值由对应的有效ID复制而来，无论是否开启了set-user-ID和set-group-ID权限位

假如某程序的实际用户ID、有效用户ID、保存set-user-ID都是1000，当其执行了root用户拥有的set-user-ID程序后，进程的用户ID将发生变化：

```
real=1000 effective=0 saved=0
```

##### 文件系统用户ID和组ID

打开文件、修改文件属主、修改文件权限之类的文件系统操作，决定操作权限的是文件系统用户ID和组ID（结合辅助组ID），通常，文件系统用户ID和组ID的值等于相应的有效用户ID和组ID（也等于实际用户ID和组ID），只要有效用户ID或组ID发生了变化，无论是通过系统调用还是执行set-user-ID或set-group-ID程序，相应的文件系统用户ID也将随之改变为同一值，只有当调用setfsuid和setfsgid时，文件系统ID与相应的有效ID不同

从严格意义上将，保留文件系统ID特性已无必要

##### 辅助组ID

用于标识进程所属的若干附加的组，新进程从其父进程继承这些ID，登录shell从系统组文件中获取其辅助组ID

##### 获取和修改进程凭证

可以利用Linux特有的proc/PID/status文件，通过对其UID、GID和Groups各行信息的检查来获取任何进程的凭证

1. 获取实际和有效ID

```
#include <unistd.h>

pid_t getuid(void);  // 返回实际用户id
pid_t geteuid(void); // 返回有效用户id
pid_t getgid(void);  // 返回实际组id
pid_t getegid(void); // 返回有效组id
```

2. 修改有效ID

```
int setuid(uid_t uid);
int setgid(gid_t gid);
// 两个函数返回值，若成功，返回0，若出错，返回-1
```

不仅可以修改进程的有效ID，还可能修改实际用户ID和保存ID，取决于进程是否拥有特权（即root）：

* 非特权进程调用setuid，仅能修改进程的有效用户ID，且其值只能与相应的实际用户ID或保存ID相等，这意味着，仅当执行set-user-ID程序时，setuid才会起作用
* 特权进程以非0参数调用setuid，其实际用户ID、有效用户ID、保存ID都被修改为指定的值，这个操作是单向的，一旦修改所有特权将丢失，之后也不能使用setuid将有效用户ID重置为0

对set-user-ID-root程序而言（其有效用户ID是0），以不可逆方式放弃进程所有特权的首选方法是使用如下系统调用：（以实际用户ID设置有效用户ID和保存ID）

```
if (-1 == setuid(getuid()))
	perror("setuid err");
```

如果set-user-ID程序的属主不是root用户，使用setuid将有效用户ID在实际用户ID和保存ID之前切换

修改有效ID的另一种方式：

```
int seteuid(uid_t uid);
int setegid(gid_t gid);
// 两个函数返回值，若成功，返回0，若出错，返回-1
```

* 非特权进程仅能将其有效用户ID修改为相应的实际ID或保存ID
* 特权进程能够将其有效用户ID修改为任意值，若将有效用户ID设置为非0值，则进程不再具有特权，但可以恢复

对于需要对特权收放自如的set-user-ID和set-group-ID程序而言，更推荐seteuid：

```
euid = geteuid(); // 先保存

if (-1 == seteuid(getuid())) // 再设置，放弃特权
	perror("seteuid error");
if (-1 == seteuid(euid))		 // 再恢复，重获特权
	perror("seteuid error");
```

3. 修改实际和有效ID

```
int setreuid(uid_t ruid, uid_t euid);
int setregid(gid_t rgid, gid_t egid);
// 两个函数返回值，若成功，返回0，若出错，返回-1
// 第一个参数是新的实际ID，第二个参数是新的有效ID
```

* 非特权进程只能将其实际用户ID设置为当前实际用户ID值或有效用户ID值，且只能将有效用户ID设置为当前实际用户ID、有效用户ID或保存ID
* 特权进程能够设置其实际用户ID和有效用户ID为任意值
* 不管进程是否拥有特权，只要如下条件之一成立，就能将保存ID设置为（新的）有效用户ID：
  * ruid不是-1（即设置实际用户ID，即便是当前值）
  * 对有效用户ID所设置的值不同于系统调用之前的实际用户ID

这为set-user-ID程序提供了一个永久放弃特权的方法：

```
setreuid(getuid(), getuid());
```

set-user-ID-root进程若有意将用户凭证和组凭证改变为任意值，应首先调用setregid，再调用setreuid，顺序不能颠倒

4. 获取实际、有效和保存ID

```
#define _GNU_SOURCE
#include <unistd.h>

int getresuid(uid_t *ruid, uid_t *euid, uid_t *suid);
int getresgid(gid_t *rgid, gid_t *egid, gid_t *sgid);
// 两个函数返回值，若成功，返回0，若出错，返回-1
```

5. 修改实际、有效和保存ID

```
int setresuid(uid_t ruid, uid_t euid, uid_t suid);
int setresgid(gid_t rgid, gid_t egid, gid_t sgid);
// 两个函数返回值，若成功，返回0，若出错，返回-1
// 若不想同时修改，将无需修改的ID设置为-1
// 两个函数都具有原子性，要么全部修改，要么全部不修改
```

* 非特权进程能够将实际用户ID、有效用户ID和保存ID的任一ID设置为实际用户ID、有效用户ID和保存ID中的任一当前值
* 特权进程能够对其实际用户ID、有效用户ID和保存ID做任意修改
* 不管系统调用是否对其他ID做了任何改动，总是将文件系统用户ID设置为与有效用户ID相同

##### 获取和修改文件系统ID

前述所有修改进程有效用户ID和组ID的系统调用都会修改相应的文件系统ID，要想文件系统ID独立于有效用户ID：

```
#include <sys/fsuid.h>

int setfsuid(uid_t fsuid);
// 总是返回先前的文件系统用户ID
int setfsgid(gid_t fsgid);
// 总是返回先前的文件系统组ID
```

Linux中，使用这两个调用已非必要，要保证移植性，应避免使用

##### 获取和修改辅助组ID

```
int getgroups(int gidsetsize, gid_t grouplist[]);
// 返回值：若成功，返回grouplist中的group id的个数，若出错，返回-1
// 若进程辅助组的数量超出gidsetsize，则将返回错误号设置为EINVAL
// 调用程序要给grouplist分配空间
// 若将gidsetsize设置为0，调用的返回值是进程辅助组的数量
```

数组大小常量NGROUPS_MAX可通过sysconf(_SC_NGROUPS_MAX)或Linux特有的/proc/sys/kernel/ngroups_max读取

特权进程能够使用如下调用修改其辅助组ID集合：

```
#define _BSD_SOURCE
#include <grp.h>

int setgroups(size_t gidsetsize, const gid_t *grouplist);
int initgroups(const char *user, gid_t group);
// 两个函数返回值，若成功，返回0，若出错，返回-1
// initgroups扫描/etc/groups文件，为user创建属组列表，以此来初始化进程的辅助组ID，参数group指定的组ID追加到辅助组ID的集合中，其主要用途是创建登录会话的程序如login
```

![WechatIMG10.jpeg](https://i.loli.net/2019/11/23/mIJoWcVshtET3AB.jpg)