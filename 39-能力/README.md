## 第39章 能力

### 能力基本原理

传统的UNIX权限模型将进程分为两类：有效用户ID为0（超级用户）的进程、其他进程；如果一个非特权用户要执行超级用户才能执行的操作，通常需要使用set-user-ID-root程序，但同时赋予了它其他操作的权限；Linux能力模型在执行安全性检测时，超级用户的权限被划分为了不同的单元，称之为能力，每个特权操作与特定能力关联；超级用户会被内核赋予所有能力

### 进程和文件能力

| capability 名称      | 描述                                                         |
| -------------------- | ------------------------------------------------------------ |
| CAP_AUDIT_CONTROL    | 启用和禁用内核审计；改变审计过滤规则；检索审计状态和过滤规则 |
| CAP_AUDIT_READ       | 允许通过 multicast netlink 套接字读取审计日志                |
| CAP_AUDIT_WRITE      | 将记录写入内核审计日志                                       |
| CAP_BLOCK_SUSPEND    | 使用可以阻止系统挂起的特性                                   |
| CAP_CHOWN            | 修改文件所有者的权限                                         |
| CAP_DAC_OVERRIDE     | 忽略文件的 DAC 访问限制                                      |
| CAP_DAC_READ_SEARCH  | 忽略文件读及目录搜索的 DAC 访问限制                          |
| CAP_FOWNER           | 忽略文件属主 ID 必须和进程用户 ID 相匹配的限制               |
| CAP_FSETID           | 允许设置文件的 setuid 位                                     |
| CAP_IPC_LOCK         | 允许锁定共享内存片段                                         |
| CAP_IPC_OWNER        | 忽略 IPC 所有权检查                                          |
| CAP_KILL             | 允许对不属于自己的进程发送信号                               |
| CAP_LEASE            | 允许修改文件锁的 FL_LEASE 标志                               |
| CAP_LINUX_IMMUTABLE  | 允许修改文件的 IMMUTABLE 和 APPEND 属性标志                  |
| CAP_MAC_ADMIN        | 允许 MAC 配置或状态更改                                      |
| CAP_MAC_OVERRIDE     | 覆盖 MAC(Mandatory Access Control)                           |
| CAP_MKNOD            | 允许使用 mknod() 系统调用                                    |
| CAP_NET_ADMIN        | 允许执行网络管理任务                                         |
| CAP_NET_BIND_SERVICE | 允许绑定到小于 1024 的端口                                   |
| CAP_NET_BROADCAST    | 允许网络广播和多播访问                                       |
| CAP_NET_RAW          | 允许使用原始套接字                                           |
| CAP_SETGID           | 允许改变进程的 GID                                           |
| CAP_SETFCAP          | 允许为文件设置任意的 capabilities                            |
| CAP_SETPCAP          | 参考 [capabilities man page](http://man7.org/linux/man-pages/man7/capabilities.7.html) |
| CAP_SETUID           | 允许改变进程的 UID                                           |
| CAP_SYS_ADMIN        | 允许执行系统管理任务，如加载或卸载文件系统、设置磁盘配额等   |
| CAP_SYS_BOOT         | 允许重新启动系统                                             |
| CAP_SYS_CHROOT       | 允许使用 chroot() 系统调用                                   |
| CAP_SYS_MODULE       | 允许插入和删除内核模块                                       |
| CAP_SYS_NICE         | 允许提升优先级及设置其他进程的优先级                         |
| CAP_SYS_PACCT        | 允许执行进程的 BSD 式审计                                    |
| CAP_SYS_PTRACE       | 允许跟踪任何进程                                             |
| CAP_SYS_RAWIO        | 允许直接访问 /devport、/dev/mem、/dev/kmem 及原始块设备      |
| CAP_SYS_RESOURCE     | 忽略资源限制                                                 |
| CAP_SYS_TIME         | 允许改变系统时钟                                             |
| CAP_SYS_TTY_CONFIG   | 允许配置 TTY 设备                                            |
| CAP_SYSLOG           | 允许使用 syslog() 系统调用                                   |
| CAP_WAKE_ALARM       | 允许触发一些能唤醒系统的东西(比如 CLOCK_BOOTTIME_ALARM 计时器) |

##### 进程能力

内核为每个进程维护三个能力集：

* 许可的：一个进程可能使用的能力
* 有效的：内核使用这些能力对进程执行权限检测
* 可继承的：进程执行一个程序时将这些权限带入许可集中

Linux特有的/proc/PID/status中的CapInh、CapPrm、CapEff三个字段可以查看任意进程这3个能力的16进制表示

fork创建的子进程会继承父进程的能力集，能力时一个线程级特性，可通过/proc/PID/task/TID/status查看

##### 文件能力

* 许可的：exec调用中可以将这组能力添加到进程的许可集中
* 有效的：仅一位，若被启用，在exec中进程的新许可集中启用的能力在进程的新有效集中也被启用
* 可继承的：这个集合与进程的可继承集取掩码确定exec之后的进程的许可集中启用的能力集

##### 进程许可和有效能力集的目的

进程的许可集定义了进程能够使用的能力，有效集定义了进程当前使用的能力，许可集为有效集定义了上限

##### 文件许可和有效能力集的目的

文件的许可集指定了在exec调用中被赋予进程的许可能力集的一组能力，有效集时一个可以被启动或禁用的标记

##### 进程和文件可继承集的目的

进程的可继承集指定了一组在exec调用之间可被赋予进程的许可能力集的能力，文件的可继承集会根据进程的可继承集取掩码（AND）来确定在exec之间被添加到进程的许可能力集的能力

### 现代能力实现

能力的完整实现要求：

* 对于每个特权操作，内核应该检查进程是否拥有相应的能力，而不是检查有效用户ID是否为0
* 内核必须提供允许获取和修改进程能力的系统调用
* 内核必须支持将能力附加给可执行文件的概念

### 在exec中转变进程能力

在执行exec期间，内核会根据进程的当前能力以及被执行的文件的能力集设置进程新能力，规则：

```
P'(permitted) = (P(inheritable) & F(inheritable)) | (F(permitted) & cap_bset)             
// 新进程的permitted有老进程的和新进程的inheritable和可执行文件的permitted及cap_bset运算得到
P'(effective) = F(effective) ? P'(permitted) : 0            
// 新进程的effective依赖可执行文件的effective位，使能和新进程的permitted一样，负责为空
P'(inheritable) = P(inheritable)    [i.e., unchanged]       
// 新进程的inheritable直接继承老进程的Inheritable

说明:
P   在执行execve函数前，进程的能力
P'  在执行execve函数后，进程的能力
F   可执行文件的能力
cap_bset 系统能力的边界值，在此处默认全为1
```

##### 能力边界值

是一种用于限制进程在exec期间能够获取的能力的安全机制：

* exec调用期间，能力边界集会与文件许可能力取AND来确定被授予新程序的许可能力
* 能力边界集时一个可以被添加到进程的可继承集中的能力的受限超集

fork创建的子进程会继承这个特性并在exec中保持，init启动时会使用一个包含了所有能力的能力边界集

准确的说，能力边界集是一个线程特性，/proc/PID/task/TID/status中的CapBnd字段可以查看

##### 保持root语义

执行一个文件时 为了保持root用户的传统语义，与该文件关联的所有能力集都会被忽略，exec调用期间，文件能力集的定义如下：

* 如果执行了set-user-ID-root程序或调用exec的进程的真实或有效用户ID为0，那么文件的可继承和许可集被定义为包含所有能力
* 如果执行了set-user-ID-root程序或调用exec的进程的有效用户ID为0，那么文件的有效位被定义为设置状态

如在执行一个set-user-ID-root程序，根据文件能力集的概念，进程的新许可和有效能力集被简化了：

```
P'(permitted) = P(inheritable) | cap_bset          
P'(effective) = P(effective)    
```

### 改变用户ID对进程能力的影响

为了与用户ID在0与非0之间切换的传统语义保持兼容，在改变进程的用户ID时，内核会完成如下操作：

* 当一个进程的有效用户ID从0变化到非0， 那么所有的E能力清零
* 当一个进程的有效用户ID从非0变化到0，那么现有的P集合拷贝到E集合
* 如果一个进行原来的真实用户ID，有效用户ID，保存设置用户ID是0，由于某些操作这些ID都变成了非0，那么所有的的P和E能力全部清理
* 如果一个文件系统的用户ID从0变成非0，那么以下的能力在E集合中清除：CAP_CHOWN, CAP_DAC_OVERRIDE,  CAP_DAC_READ_SEARCH,  CAP_FOWNER,  CAP_FSETID,  CAP_LINUX_IMMUTABLE  (since  Linux  2.2.30),  CAP_MAC_OVERRIDE,  CAP_MKNOD，如果一个文件系统的用户ID从0变成非0，那么在P集合中使能的能力将设置到E集合中

### 用编程的方式改变进程能力

修改进程能力的规则：

* 如果进程的有效集中没有CAP_SETPCAP能力，新的可继承集必须是既有可继承集、许可集的一个子集
* 新的可继承集必须是既有可继承集、能力边界集的一个子集
* 新许可集必须是既有许可集的一个子集
* 新的有效集只能包含位于新许可集中的能力

### 创建仅包含能力的环境

从内核2.6.26开始，当在内核中启用文件能力时，Linux提供了securebits机制，他可以控制一组进程级别的标记，通过这组标记可以分别启用或禁用针对root的三种特殊情况的各种处理

```
SECBIT_KEEP_CAPS: 当一个或多个用户ID为0的进程将其所有的用户ID设置为非0时不要删除许可权限
SECBIT_NO_SETUID_FIXUP: 有效用户或文件系统用户ID在0和非0之间切换时不要改变能力
SECBIT_NOROOT: 真实或有效用户ID为0的进程调用exec或执行set-user-ID-root程序时不要赋予其能力

SECBIT_KEEP_CAPS_LOCKED: 锁住SECBIT_KEEP_CAPS
SECBIT_NO_SETUID_FIXUP_LOCKED: 锁住SECBIT_NO_SETUID_FIXUP
SECBIT_NOROOT_LOCKED: 锁住SECBIT_NOROOT
```

fork创建的子进程会继承securebits的标记，调用exec期间，除了SECBIT_KEEP_CAPS之外的标记会保留

### 发现程序所需的能力

当一个程序不是set-user-ID-root程序时，如何确定将哪些能力赋予它：

* 使用strace检查哪个系统调用的错误号是EPERM，但偶尔其他原因也会导致这个错误
* 使用一个内核探针在内核被要求执行能力检查时产生监控输出

### 不具备文件能力的老式内核和系统

Linux在下面两个场景中不支持文件能力：

1. Linux2.6.24之前的内核
2. Linux2.6.24以后的内核，但没有指定CONFIG_SECURITY_FILE_CAPABILITIES选项

一个在有效集中包含CAP_SETPCAP能力的进程能够修改处自身之外的其他进程的能力

通过Linux特有的/proc/sys/kernel/cap-bound可以查看系统级别的能力边界集，进程必须具备CAP_SYS_MODULE能力才能修改此文件的内容，只有init进程才能开启这个掩码的位