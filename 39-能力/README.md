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

