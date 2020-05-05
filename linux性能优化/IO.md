# IO

### Linux 文件系统是怎么工作的？

##### 索引节点和目录项

Linux 文件系统为每个文件都分配两个数据结构，索引节点（index node）和目录项（directory entry）。它们主要用来记录文件的元信息和目录结构。索引节点是每个文件的唯一标志，而目录项维护的正是文件系统的树状结构。目录项和索引节点的关系是多对一，一个文件可以有多个别名

* 索引节点，简称为 inode，用来记录文件的元数据，比如 inode 编号、文件大小、访问权限、修改日期、数据的位置等。索引节点和文件一一对应，它跟文件内容一样，都会被持久化存储到磁盘中。索引节点同样占用磁盘空间
* 目录项，简称为 dentry，用来记录文件的名字、索引节点指针以及与其他目录项的关联关系。多个关联的目录项，就构成了文件系统的目录结构。不过，不同于索引节点，目录项是由内核维护的一个内存数据结构，所以通常也被叫做目录项缓存

磁盘读写的最小单位是扇区，然而扇区只有 512B 大小，如果每次都读写这么小的单位，效率一定很低。所以，文件系统又把连续的扇区组成了逻辑块，然后每次都以逻辑块为最小单元，来管理数据。常见的逻辑块大小为 4KB，由连续的 8 个扇区组成

![img](https://static001.geekbang.org/resource/image/32/47/328d942a38230a973f11bae67307be47.png)

超级块：存储整个文件系统的状态。索引节点区：存储索引节点。数据块区：存储文件数据

##### 虚拟文件系统

目录项、索引节点、逻辑块以及超级块，构成了 Linux 文件系统的四大基本要素。为了支持各种不同的文件系统，Linux 内核在用户进程和文件系统的中间，又引入了一个抽象层：虚拟文件系统 VFS（Virtual File System）

![img](https://static001.geekbang.org/resource/image/72/12/728b7b39252a1e23a7a223cdf4aa1612.png)

文件系统可以分为三类：

* 第一类是基于磁盘的文件系统：把数据直接存储在计算机本地挂载的磁盘中。常见的 Ext4、XFS、OverlayFS 等，都是这类文件系统
* 第二类是基于内存的文件系统：虚拟文件系统。不需要任何磁盘分配存储空间，但会占用内存。/proc 文件系统，就是一种最常见的虚拟文件系统。/sys 文件系统也属于这一类，主要向用户空间导出层次化的内核对象
* 第三类是网络文件系统：用来访问其他计算机数据的文件系统，比如 NFS、SMB、iSCSI 等

这些文件系统，要先挂载到 VFS 目录树中的某个子目录（称为挂载点），然后才能访问其中的文件

##### 文件系统 I/O

把文件系统挂载到挂载点后，就能通过挂载点，再去访问它管理的文件了。VFS 提供了一组标准的文件访问接口。文件读写方式的各种差异，导致 I/O 的分类多种多样。最常见的有四种：

* 第一种，根据是否利用标准库缓存，分为：
  * 缓冲 I/O，是指利用标准库缓存来加速文件的访问，而标准库内部再通过系统调度访问文件
  * 非缓冲 I/O，是指直接通过系统调用来访问文件，不再经过标准库缓存

“缓冲”，是指标准库内部实现的缓存，无论缓冲 I/O 还是非缓冲 I/O，它们最终还是要经过系统调用来访问文件。系统调用后，还会通过页缓存，来减少磁盘的 I/O 操作

* 第二种，据是否利用操作系统的页缓存，分为：
  * 直接 I/O，是指跳过操作系统的页缓存，直接跟文件系统交互来访问文件
  * 非直接 I/O 正好相反，文件读写时，先要经过系统的页缓存，然后再由内核或额外的系统调用，真正写入磁盘

实现直接 I/O，需要在系统调用中，指定 O_DIRECT 标志。如果没有设置过，默认的是非直接 I/O。直接 I/O、非直接 I/O，本质上还是和文件系统交互。如果是在数据库等场景中，跳过文件系统读写磁盘的情况，也就是通常所说的裸 I/O

* 第三种，根据应用程序是否阻塞自身运行，分为：
  * 阻塞 I/O，是指应用程序执行 I/O 操作后，如果没有获得响应，就会阻塞当前线程
  * 非阻塞 I/O，是指应用程序执行 I/O 操作后，不会阻塞当前的线程，可以继续执行其他的任务，随后再通过轮询或者事件通知的形式，获取调用的结果

访问管道或者网络套接字时，设置 O_NONBLOCK 标志，表示用非阻塞方式访问；而如果不做任何设置，默认的就是阻塞访问

* 第四种，根据是否等待响应结果，分为：
  * 同步 I/O，是指应用程序执行 I/O 操作后，要一直等到整个 I/O 完成后，才能获得 I/O 响应
  * 异步 I/O，是指应用程序执行 I/O 操作后，不用等待完成和完成后的响应，而是继续执行就可以。等到这次 I/O 完成后，响应会用事件通知的方式，告诉应用程序

在操作文件时，如果设置了 O_SYNC 或者 O_DSYNC 标志，就代表同步 I/O。如果设置了 O_DSYNC，就要等文件数据写入磁盘后，才能返回；而 O_SYNC，则是在 O_DSYNC 基础上，要求文件元数据也要写入磁盘后，才能返回。再比如，在访问管道或者网络套接字时，设置了 O_ASYNC 选项后，相应的 I/O 就是异步 I/O。这样，内核会再通过 SIGIO 或者 SIGPOLL，来通知进程文件是否可读写

阻塞 / 非阻塞和同步 / 异步，其实就是两个不同角度的 I/O 划分方式。它们描述的对象也不同，阻塞 / 非阻塞针对的是 I/O 调用者（即应用程序），而同步 / 异步针对的是 I/O 执行者（即系统）

##### 性能观测

查看容量

```
$ df -h /dev/sda1 
Filesystem      Size  Used Avail Use% Mounted on 
/dev/sda1        29G  3.1G   26G  11% / 

// -i 可以查看索引节点的使用情况，索引节点空间不足，但磁盘空间充足时，很可能就是过多小文件导致的
$ df -i /dev/sda1 
Filesystem      Inodes  IUsed   IFree IUse% Mounted on 
/dev/sda1      3870720 157460 3713260    5% / 
```

查看缓存

```
// free 输出的 Cache，是页缓存和可回收 Slab 缓存的和，可以从 /proc/meminfo ，直接得到它们的大小

$ cat /proc/meminfo | grep -E "SReclaimable|Cached" 
Cached:           748316 kB 
SwapCached:            0 kB 
SReclaimable:     179508 kB 

内核使用 Slab 机制，管理目录项和索引节点的缓存。/proc/meminfo 只给出了 Slab 的整体大小，具体到每一种 Slab 缓存，还要查看 /proc/slabinfo 这个文件

// 查看所有目录项和各种文件系统索引节点的缓存情况
$ cat /proc/slabinfo | grep -E '^#|dentry|inode' 
# name            <active_objs> <num_objs> <objsize> <objperslab> <pagesperslab> : tunables <limit> <batchcount> <sharedfactor> : slabdata <active_slabs> <num_slabs> <sharedavail> 
xfs_inode              0      0    960   17    4 : tunables    0    0    0 : slabdata      0      0      0 
... 
ext4_inode_cache   32104  34590   1088   15    4 : tunables    0    0    0 : slabdata   2306   2306      0hugetlbfs_inode_cache     13     13    624   13    2 : tunables    0    0    0 : slabdata      1      1      0 
sock_inode_cache    1190   1242    704   23    4 : tunables    0    0    0 : slabdata     54     54      0 
shmem_inode_cache   1622   2139    712   23    4 : tunables    0    0    0 : slabdata     93     93      0 
proc_inode_cache    3560   4080    680   12    2 : tunables    0    0    0 : slabdata    340    340      0 
inode_cache        25172  25818    608   13    2 : tunables    0    0    0 : slabdata   1986   1986      0 
dentry             76050 121296    192   21    1 : tunables    0    0    0 : slabdata   5776   5776      0 

dentry 行表示目录项缓存，inode_cache 行，表示 VFS 索引节点缓存，其余的则是各种文件系统的索引节点缓存
实际性能分析中，更常使用 slabtop  ，来找到占用内存最多的缓存类型

# 按下c按照缓存大小排序，按下a按照活跃对象数排序 
$ slabtop 
Active / Total Objects (% used)    : 277970 / 358914 (77.4%) 
Active / Total Slabs (% used)      : 12414 / 12414 (100.0%) 
Active / Total Caches (% used)     : 83 / 135 (61.5%) 
Active / Total Size (% used)       : 57816.88K / 73307.70K (78.9%) 
Minimum / Average / Maximum Object : 0.01K / 0.20K / 22.88K 

  OBJS ACTIVE  USE OBJ SIZE  SLABS OBJ/SLAB CACHE SIZE NAME 
69804  23094   0%    0.19K   3324       21     13296K dentry 
16380  15854   0%    0.59K   1260       13     10080K inode_cache 
58260  55397   0%    0.13K   1942       30      7768K kernfs_node_cache 
   485    413   0%    5.69K     97        5      3104K task_struct 
  1472   1397   0%    2.00K     92       16      2944K kmalloc-2048 
  
目录项dentry和索引节点inode_cache占用了最多的 Slab 缓存。不过加起来也只有 23MB 左右  
```

### Linux 磁盘I/O是怎么工作的

##### 磁盘

磁盘是可以持久化存储的设备，根据存储介质的不同，常见磁盘可以分为两类：

* 机械磁盘（Hard Disk Driver）：也称为硬盘驱动器，缩写为 HDD。主要由盘片和读写磁头组成，数据就存储在盘片的环状磁道中。在读写数据前，需要移动读写磁头，定位到数据所在的磁道，然后才能访问数据。最小读写单位是扇区，一般大小为 512 字节
* 固态磁盘（Solid State Disk）：缩写为 SSD，由固态电子元器件组成。固态磁盘不需要磁道寻址，所以，不管是连续 I/O，还是随机 I/O 的性能，都比机械磁盘要好得多。最小读写单位是页，通常大小是 4KB、8KB 等

按照接口来分类，可以把硬盘分为：

* IDE（Integrated Drive Electronics）
* SCSI（Small Computer System Interface） 
* SAS（Serial Attached SCSI） 
* SATA（Serial ATA） 
* FC（Fibre Channel） 等

不同的接口，往往分配不同的设备名称。比如， IDE 设备会分配一个 hd 前缀的设备名，SCSI 和 SATA 设备会分配一个 sd 前缀的设备名。如果是多块同类型的磁盘，就会按照 a、b、c 等的字母顺序来编号。作为独立磁盘设备来使用。磁盘还会根据需要，划分为不同的逻辑分区，每个分区再用数字编号。比如 /dev/sda ，可以分成两个分区 /dev/sda1 和 /dev/sda2

一个比较常用的架构，是把多块磁盘组合成一个逻辑磁盘，构成冗余独立磁盘阵列，也就是 RAID（Redundant Array of Independent Disks），从而可以提高数据访问的性能，并且增强数据存储的可靠性

在 Linux 中，磁盘实际上是作为一个块设备来管理的，也就是以块为单位读写数据，并且支持随机读写。每个块设备都会被赋予两个设备号，主设备号用在驱动程序中，用来区分设备类型；而次设备号则是用来给多个同类设备编号

##### 通用块层

其实是处在文件系统和磁盘驱动中间的一个块设备抽象层。它主要有两个功能 ：

* 第一个功能跟虚拟文件系统的功能类似。向上，为文件系统和应用程序，提供访问块设备的标准接口；向下，把各种异构的磁盘设备抽象为统一的块设备，并提供统一框架来管理这些设备的驱动程序
* 第二个功能，通用块层还会给文件系统和应用程序发来的 I/O 请求排队，并通过重新排序、请求合并等方式，提高磁盘读写的效率

对 I/O 请求排序的过程，也就是我们熟悉的 I/O 调度。事实上，Linux 内核支持四种 I/O 调度算法：

* 第一种 NONE ，完全不使用任何 I/O 调度器，常用在虚拟机中（此时磁盘 I/O 调度完全由物理机负责）
* 第二种 NOOP ，是一个先入先出的队列，只做一些最基本的请求合并，常用于 SSD 磁盘
* 第三种 CFQ（Completely Fair Scheduler），也被称为完全公平调度器，是现在很多发行版的默认 I/O 调度器，它为每个进程维护了一个 I/O 调度队列，并按照时间片来均匀分布每个进程的 I/O 请求
* 第四种DeadLine 调度算法，分别为读、写请求创建了不同的 I/O 队列，可以提高机械磁盘的吞吐量，并确保达到最终期限（deadline）的请求被优先处理。多用在 I/O 压力比较重的场景，比如数据库等

##### I/O 栈

把 Linux 存储系统的 I/O 栈，由上到下分为三个层次，分别是文件系统层、通用块层和设备层。这三个 I/O 层的关系如下图所示，这也是 Linux 存储系统的 I/O 栈全景图

![img](https://static001.geekbang.org/resource/image/14/b1/14bc3d26efe093d3eada173f869146b1.png)

根据这张 I/O 栈的全景图，存储系统 I/O 的工作原理：

* 文件系统层：包括虚拟文件系统和其他各种文件系统的具体实现。它为上层的应用程序，提供标准的文件访问接口；对下会通过通用块层，来存储和管理磁盘数据
* 通用块层：包括块设备 I/O 队列和 I/O 调度器。向上为文件系统和应用程序，提供访问了块设备的标准接口；向下把各种异构的磁盘设备，抽象为统一的块设备，并会对文件系统和应用程序发来的 I/O 请求，进行重新排序、请求合并等，提高了磁盘访问的效率
* 设备层：包括存储设备和相应的驱动程序，负责最终物理设备的 I/O 操作

##### 磁盘性能指标

* 使用率：磁盘处理 I/O 的时间百分比。过高的使用率（比如超过 80%），通常意味着磁盘 I/O 存在性能瓶颈。当使用率是 100% 的时候，磁盘依然有可能接受新的 I/O 请求
* 饱和度：磁盘处理 I/O 的繁忙程度。过高的饱和度，意味着磁盘存在严重的性能瓶颈。当饱和度为 100% 时，磁盘无法接受新的 I/O 请求
* IOPS（Input/Output Per Second）：每秒的 I/O 请求数
* 吞吐量：每秒的 I/O 请求大小
* 响应时间： I/O 请求从发出到收到响应的间隔时间

分析工具：

- fio：测试磁盘的 IOPS、吞吐量以及响应时间等核心指标

需要测试出，不同 I/O 大小（一般是 512B 至 1MB 中间的若干值）分别在随机读、顺序读、随机写、顺序写等各种场景下的性能情况。用性能工具得到的这些指标，可以作为后续分析应用程序性能的依据。一旦发生性能问题，你就可以把它们作为磁盘性能的极限值，进而评估磁盘 I/O 的使用情况

##### 磁盘IO观测

分析工具：

* iostat：提供了每个磁盘的使用率、IOPS、吞吐量等各种常见的性能指标，这些指标实际上来自  /proc/diskstats

![img](https://static001.geekbang.org/resource/image/cf/8d/cff31e715af51c9cb8085ce1bb48318d.png)

其中：

* %util ：磁盘 I/O 使用率
* r/s+  w/s：IOPS
* rkB/s+wkB/s ：吞吐量
* r_await+w_await ：响应时间

饱和度通常也没有其他简单的观测方法，不过可以把观测到的，平均请求队列长度或者读写请求完成的等待时间，跟基准测试的结果（比如通过 fio）进行对比，综合评估磁盘的饱和情况

##### 进程 I/O 观测

iostat 只提供磁盘整体的 I/O 性能数据，要观察进程的 I/O 情况，可以使用 pidstat 和 iotop 这两个工具

```

$ pidstat -d 1 
13:39:51      UID       PID   kB_rd/s   kB_wr/s kB_ccwr/s iodelay  Command 
13:39:52      102       916      0.00      4.00      0.00       0  rsyslogd

用户 ID（UID）和进程 ID（PID）
每秒读取的数据大小（kB_rd/s） ，单位是 KB
每秒发出的写请求数据大小（kB_wr/s） ，单位是 KB
每秒取消的写请求数据大小（kB_ccwr/s） ，单位是 KB
块 I/O 延迟（iodelay），包括等待同步块 I/O 和换入块 I/O 结束的时间，单位是时钟周期
```

分析工具：

* iotop：一个类似于 top 的工具，可以按照 I/O 大小对进程排序，然后找到 I/O 较大的那些进程

```
$ iotop
Total DISK READ :       0.00 B/s | Total DISK WRITE :       7.85 K/s 
Actual DISK READ:       0.00 B/s | Actual DISK WRITE:       0.00 B/s 
  TID  PRIO  USER     DISK READ  DISK WRITE  SWAPIN     IO>    COMMAND 
15055 be/3 root        0.00 B/s    7.85 K/s  0.00 %  0.00 % systemd-journald 

前两行分别表示，进程的磁盘读写大小总数和磁盘真实的读写大小总数。因为缓存、缓冲区、I/O 合并等因素的影响，它们可能并不相等

线程 ID
I/O 优先级
每秒读磁盘的大小
每秒写磁盘的大小
换入和等待 I/O 的时钟百分比等
```

### 如何找出狂打日志的“内鬼”？

##### 案例分析

```
$ docker run -v /tmp:/tmp --name=app -itd feisky/logapp 
// 确认app.py 的进程已经启动
$ ps -ef | grep /app.py 
root     18940 18921 73 14:41 pts/0    00:00:02 python /app.py 


# 按1切换到每个CPU的使用情况 
$ top 
top - 14:43:43 up 1 day,  1:39,  2 users,  load average: 2.48, 1.09, 0.63 
Tasks: 130 total,   2 running,  74 sleeping,   0 stopped,   0 zombie 
%Cpu0  :  0.7 us,  6.0 sy,  0.0 ni,  0.7 id, 92.7 wa,  0.0 hi,  0.0 si,  0.0 st 
%Cpu1  :  0.0 us,  0.3 sy,  0.0 ni, 92.3 id,  7.3 wa,  0.0 hi,  0.0 si,  0.0 st 
KiB Mem :  8169308 total,   747684 free,   741336 used,  6680288 buff/cache 
KiB Swap:        0 total,        0 free,        0 used.  7113124 avail Mem 

  PID USER      PR  NI    VIRT    RES    SHR S  %CPU %MEM     TIME+ COMMAND 
18940 root      20   0  656108 355740   5236 R   6.3  4.4   0:12.56 python 
1312 root      20   0  236532  24116   9648 S   0.3  0.3   9:29.80 python3 

问题1. CPU0 的使用率非常高，它的系统 CPU 使用率（sys%）为 6%，而 iowait 超过了 90%。这说明 CPU0 上，可能正在运行 I/O 密集型的进程
问题2. 总内存 8G，剩余内存只有 730 MB，而 Buffer/Cache 占用内存高达 6GB 之多，这说明内存主要被缓存占用。虽然大部分缓存可回收，还是得了解下缓存的去处

// 停止 top 命令，再运行 iostat 命令，观察 I/O 的使用情况
# -d表示显示I/O性能指标，-x表示显示扩展统计（即所有I/O指标） 
$ iostat -x -d 1 
Device            r/s     w/s     rkB/s     wkB/s   rrqm/s   wrqm/s  %rrqm  %wrqm r_await w_await aqu-sz rareq-sz wareq-sz  svctm  %util 
loop0            0.00    0.00      0.00      0.00     0.00     0.00   0.00   0.00    0.00    0.00   0.00     0.00     0.00   0.00   0.00 
sdb              0.00    0.00      0.00      0.00     0.00     0.00   0.00   0.00    0.00    0.00   0.00     0.00     0.00   0.00   0.00 
sda              0.00   64.00      0.00  32768.00     0.00     0.00   0.00   0.00    0.00 7270.44 1102.18     0.00   512.00  15.50  99.20

磁盘 sda 的 I/O 使用率已经高达 99%，很可能已经接近 I/O 饱和。再看前面的各个指标，每秒写磁盘请求数是 64 ，写大小是 32 MB，写请求的响应时间为 7 秒，而请求队列长度则达到了 1100。超慢的响应时间和特长的请求队列长度，进一步验证了 I/O 已经饱和的猜想。此时，sda 磁盘已经遇到了严重的性能瓶颈

// 使用 pidstat 加上 -d 参数，就可以显示每个进程的 I/O 情况
$ pidstat -d 1 
15:08:35      UID       PID   kB_rd/s   kB_wr/s kB_ccwr/s iodelay  Command 
15:08:36        0     18940      0.00  45816.00      0.00      96  python 

15:08:36      UID       PID   kB_rd/s   kB_wr/s kB_ccwr/s iodelay  Command 
15:08:37        0       354      0.00      0.00      0.00     350  jbd2/sda1-8 
15:08:37        0     18940      0.00  46000.00      0.00      96  python 
15:08:37        0     20065      0.00      0.00      0.00    1503  kworker/u4:2 

python 进程的写比较大，而且每秒写的数据超过 45 MB，比上面 iostat 发现的 32MB 的结果还要大。正是 python 进程导致了 I/O 瓶颈。kworker 和 jbd2 ）的延迟，居然比 python 进程还大很多，kworker 是一个内核线程，而 jbd2 是 ext4 文件系统中，用来保证数据完整性的内核线程。他们都是保证文件系统基本功能的内核线程

// 运行 strace 命令，并通过 -p 18940 指定 python 进程的 PID 号

$ strace -p 18940 
strace: Process 18940 attached 
...
mmap(NULL, 314576896, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x7f0f7aee9000 
mmap(NULL, 314576896, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x7f0f682e8000 
write(3, "2018-12-05 15:23:01,709 - __main"..., 314572844 
) = 314572844 
munmap(0x7f0f682e8000, 314576896)       = 0 
write(3, "\n", 1)                       = 1 
munmap(0x7f0f7aee9000, 314576896)       = 0 
close(3)                                = 0 
stat("/tmp/logtest.txt.1", {st_mode=S_IFREG|0644, st_size=943718535, ...}) = 0 

从 write() 系统调用上，进程向文件描述符编号为 3 的文件中，写入了 300MB 的数据

// 运行下面的 lsof 命令，看看进程 18940 都打开了哪些文件
$ lsof -p 18940 
COMMAND   PID USER   FD   TYPE DEVICE  SIZE/OFF    NODE NAME 
python  18940 root  cwd    DIR   0,50      4096 1549389 / 
python  18940 root  rtd    DIR   0,50      4096 1549389 / 
… 
python  18940 root    2u   CHR  136,0       0t0       3 /dev/pts/0 
python  18940 root    3w   REG    8,1 117944320     303 /tmp/logtest.txt 

FD 表示文件描述符号，TYPE 表示文件类型，NAME 表示文件路径
进程 18940 以每次 300MB 的速度，在“疯狂”写日志，而日志文件的路径是 /tmp/logtest.txt
```

### 为什么我的磁盘I/O延迟很高？

##### 案例分析

分析工具：

* filetop：bcc 软件包的一部分，主要跟踪内核中文件的读写情况，并输出线程 ID（TID）、读写大小、读写类型以及文件名称
* opensnoop：同属于 bcc 软件包，可以动态跟踪内核中的 open 系统调用

```
// 第一个终端中执行
$ docker run --name=app -p 10000:80 -itd feisky/word-pop 

// 第二个终端中执行
$ curl http://192.168.0.10:10000/ 
hello world 
// 继续访问，发现很长时间没有响应
$ curl http://192.168.0.10:1000/popularity/word 
// 为了避免分析过程中 curl 请求突然结束，把 curl 命令放到一个循环里执行；这次我们还要加一个 time 命令，观察每次的执行时间
$ while true; do time curl http://192.168.0.10:10000/popularity/word; sleep 1; done 

// 第一个终端
$ top 
top - 14:27:02 up 10:30,  1 user,  load average: 1.82, 1.26, 0.76 
Tasks: 129 total,   1 running,  74 sleeping,   0 stopped,   0 zombie 
%Cpu0  :  3.5 us,  2.1 sy,  0.0 ni,  0.0 id, 94.4 wa,  0.0 hi,  0.0 si,  0.0 st 
%Cpu1  :  2.4 us,  0.7 sy,  0.0 ni, 70.4 id, 26.5 wa,  0.0 hi,  0.0 si,  0.0 st 
KiB Mem :  8169300 total,  3323248 free,   436748 used,  4409304 buff/cache 
KiB Swap:        0 total,        0 free,        0 used.  7412556 avail Mem 

  PID USER      PR  NI    VIRT    RES    SHR S  %CPU %MEM     TIME+ COMMAND 
12280 root      20   0  103304  28824   7276 S  14.0  0.4   0:08.77 python 
   16 root      20   0       0      0      0 S   0.3  0.0   0:09.22 ksoftirqd/1 
1549 root      20   0  236712  24480   9864 S   0.3  0.3   3:31.38 python3 

问题1. 两个 CPU 的 iowait 都非常高
问题2.  python 进程的 CPU 使用率稍微有点高，达到了 14%

// 第一个终端停止 top 命令；然后执行下面的 ps 命令
$ ps aux | grep app.py 
root     12222  0.4  0.2  96064 23452 pts/0    Ss+  14:37   0:00 python /app.py 
root     12280 13.9  0.3 102424 27904 pts/0    Sl+  14:37   0:09 /usr/local/bin/python /app.py 

CPU 使用率较高的进程正是12280

// 第一个终端运行 iostat 命令，-d 显示出 I/O 的性能指标；-x 显示出扩展统计信息（即显示所有 I/O 指标）
$ iostat -d -x 1
Device            r/s     w/s     rkB/s     wkB/s   rrqm/s   wrqm/s  %rrqm  %wrqm r_await w_await aqu-sz rareq-sz wareq-sz  svctm  %util 
loop0            0.00    0.00      0.00      0.00     0.00     0.00   0.00   0.00    0.00    0.00   0.00     0.00     0.00   0.00   0.00 
sda              0.00   71.00      0.00  32912.00     0.00     0.00   0.00   0.00    0.00 18118.31 241.89     0.00   463.55  13.86  98.40 

磁盘 sda 的 I/O 使用率已经达到 98% ，接近饱和了。而且，写请求的响应时间高达 18 秒，每秒的写数据为 32 MB，显然写磁盘碰到了瓶颈，这些 I/O 请求到底是哪些进程导致的呢？

// 第一个终端运行
$ pidstat -d 1 
14:39:14      UID       PID   kB_rd/s   kB_wr/s kB_ccwr/s iodelay  Command 
14:39:15        0     12280      0.00 335716.00      0.00       0  python 

先用 strace 确认它是不是在写文件，再用 lsof 找出文件描述符对应的文件即可

// 第一个终端运行，strace -p PID后加上-f，多进程和多线程都可以跟踪
$ strace -p 12280 
strace: Process 12280 attached 
select(0, NULL, NULL, NULL, {tv_sec=0, tv_usec=567708}) = 0 (Timeout) 
stat("/usr/local/lib/python3.7/importlib/_bootstrap.py", {st_mode=S_IFREG|0644, st_size=39278, ...}) = 0 
stat("/usr/local/lib/python3.7/importlib/_bootstrap.py", {st_mode=S_IFREG|0644, st_size=39278, ...}) = 0 

// 没有write调用
$ strace -p 12280 2>&1 | grep write 


# 切换到工具目录 
$ cd /usr/share/bcc/tools 

# -C 选项表示输出新内容时不清空屏幕 
$ ./filetop -C 

TID    COMM             READS  WRITES R_Kb    W_Kb    T FILE 
514    python           0      1      0       2832    R 669.txt 
514    python           0      1      0       2490    R 667.txt 
514    python           0      1      0       2685    R 671.txt 
514    python           0      1      0       2392    R 670.txt 
514    python           0      1      0       2050    R 672.txt 

...

TID    COMM             READS  WRITES R_Kb    W_Kb    T FILE 
514    python           2      0      5957    0       R 651.txt 
514    python           2      0      5371    0       R 112.txt 
514    python           2      0      4785    0       R 861.txt 
514    python           2      0      4736    0       R 213.txt 
514    python           2      0      4443    0       R 45.txt 

filetop 输出了 8 列内容，分别是线程 ID、线程命令行、读写次数、读写的大小（单位 KB）、文件类型以及读写的文件名称

每隔一段时间，线程号为 514 的 python 应用就会先写入大量的 txt 文件，再大量地读

// 第一个终端运行如下命令查看514线程属于哪个进程
$ ps -efT | grep 514
root     12280  514 14626 33 14:47 pts/0    00:00:05 /usr/local/bin/python /app.py 

filetop 只给出了文件名称，却没有文件路径

// opensnoop 动态跟踪内核中的 open 系统调用
$ opensnoop 
12280  python              6   0 /tmp/9046db9e-fe25-11e8-b13f-0242ac110002/650.txt 
12280  python              6   0 /tmp/9046db9e-fe25-11e8-b13f-0242ac110002/651.txt 
12280  python              6   0 /tmp/9046db9e-fe25-11e8-b13f-0242ac110002/652.txt 

应用会动态生成一批文件，用来临时存储数据，用完就会删除它们。正是这些文件读写，引发了 I/O 的性能瓶颈，导致整个处理过程非常慢
```

### 一个SQL查询要15秒，这是怎么回事？

##### 案例分析

```
// 第一个终端
$ curl http://127.0.0.1:10000/
Index Page

// 接着初始化数据库，并插入 10000 条商品信息
$ make init
docker exec -i mysql mysql -uroot -P3306 < tables.sql
curl http://127.0.0.1:10000/db/insert/products/10000
insert 10000 lines

// 第二个终端，访问一下商品搜索的接口，看看能不能找到想要的商品
$ curl http://192.168.0.10:10000/products/geektime
Got data: () in 15.364538192749023 sec

返回的是空数据，而且处理时间超过 15 秒

// 第二个终端继续执行
$ while true; do curl http://192.168.0.10:10000/products/geektime; sleep 5; done

// 第一个终端
$ top
top - 12:02:15 up 6 days,  8:05,  1 user,  load average: 0.66, 0.72, 0.59
Tasks: 137 total,   1 running,  81 sleeping,   0 stopped,   0 zombie
%Cpu0  :  0.7 us,  1.3 sy,  0.0 ni, 35.9 id, 62.1 wa,  0.0 hi,  0.0 si,  0.0 st
%Cpu1  :  0.3 us,  0.7 sy,  0.0 ni, 84.7 id, 14.3 wa,  0.0 hi,  0.0 si,  0.0 st
KiB Mem :  8169300 total,  7238472 free,   546132 used,   384696 buff/cache
KiB Swap:        0 total,        0 free,        0 used.  7316952 avail Mem

  PID USER      PR  NI    VIRT    RES    SHR S  %CPU %MEM     TIME+ COMMAND
27458 999       20   0  833852  57968  13176 S   1.7  0.7   0:12.40 mysqld
27617 root      20   0   24348   9216   4692 S   1.0  0.1   0:04.40 python
 1549 root      20   0  236716  24568   9864 S   0.3  0.3  51:46.57 python3
22421 root      20   0       0      0      0 I   0.3  0.0   0:01.16 kworker/u

两个 CPU 的 iowait 都比较高，特别是 CPU0

// 在第一个终端，按下 Ctrl+C，停止 top 命令；然后，执行下面的 iostat 命令
$ iostat -d -x 1
Device            r/s     w/s     rkB/s     wkB/s   rrqm/s   wrqm/s  %rrqm  %wrqm r_await w_await aqu-sz rareq-sz wareq-sz  svctm  %util
...
sda            273.00    0.00  32568.00      0.00     0.00     0.00   0.00   0.00    7.90    0.00   1.16   119.30     0.00   3.56  97.20

磁盘 sda 每秒的读数据为 32 MB， 而 I/O 使用率高达 97% ，接近饱和，磁盘 sda 的读取确实碰到了性能瓶颈

// 按下 Ctrl+C 停止 iostat 命令，然后运行下面的 pidstat 命令，观察进程的 I/O 情况

# -d选项表示展示进程的I/O情况
$ pidstat -d 1
12:04:11      UID       PID   kB_rd/s   kB_wr/s kB_ccwr/s iodelay  Command
12:04:12      999     27458  32640.00      0.00      0.00       0  mysqld
12:04:12        0     27617      4.00      4.00      0.00       3  python
12:04:12        0     27864      0.00      4.00      0.00       0  systemd-journal

PID 为 27458 的 mysqld 进程正在进行大量的读，而且读取速度是 32 MB/s，跟刚才 iostat 的发现一致。磁盘 I/O 瓶颈的根源，即 mysqld 进程

// 为了不漏掉这些线程的数据读取情况，加上 -f 参数
$ strace -f -p 27458
[pid 28014] read(38, "934EiwT363aak7VtqF1mHGa4LL4Dhbks"..., 131072) = 131072
[pid 28014] read(38, "hSs7KBDepBqA6m4ce6i6iUfFTeG9Ot9z"..., 20480) = 20480
[pid 28014] read(38, "NRhRjCSsLLBjTfdqiBRLvN9K6FRfqqLm"..., 131072) = 131072
[pid 28014] read(38, "AKgsik4BilLb7y6OkwQUjjqGeCTQTaRl"..., 24576) = 24576
[pid 28014] read(38, "hFMHx7FzUSqfFI22fQxWCpSnDmRjamaW"..., 131072) = 131072
[pid 28014] read(38, "ajUzLmKqivcDJSkiw7QWf2ETLgvQIpfC"..., 20480) = 20480


// 显示27458进程里面包含的线程，-t表示显示线程，-a表示显示命令行参数
$ pstree -t -a -p 27458
mysqld,27458 --log_bin=on --sync_binlog=1
...
  ├─{mysqld},27922
  ├─{mysqld},27923
  └─{mysqld},28014
  

$ lsof -p 27458
COMMAND  PID USER   FD   TYPE DEVICE SIZE/OFF NODE NAME
...
​mysqld  27458      999   38u   REG    8,1 512440000 2601895 /var/lib/mysql/test/products.MYD

mysqld 进程确实打开了大量文件，而根据文件描述符（FD）的编号，描述符为 38 的是一个路径为 /var/lib/mysql/test/products.MYD 的文件。38 后面的 u 表示， mysqld 以读写的方式访问文件
没有索引导致的慢查询问题，需要添加索引
```

### Redis响应严重延迟，如何解决





##### 案例分析

```
// 第二个终端，使用 curl 工具，访问应用首页
$ curl http://192.168.0.10:10000/
hello redis

// 继续插入5000条数据，可以根据磁盘的类型适当调整，比如使用SSD时可以调大，而HDD可以适当调小
$ curl http://192.168.0.10:10000/init/5000
{"elapsed_seconds":30.26814079284668,"keys_initialized":5000}

// 访问应用的缓存查询接口
$ curl http://192.168.0.10:10000/get_cache
{"count":1677,"data":["d97662fa-06ac-11e9-92c7-0242ac110002",...],"elapsed_seconds":10.545469760894775,"type":"good"}

调用居然要花 10 秒

// 为了避免分析过程中客户端的请求结束，在进行性能分析前，我们先要把 curl 命令放到一个循环里来执行
$ while true; do curl http://192.168.0.10:10000/get_cache; done

// 终端一中执行 top 命令
$ top
top - 12:46:18 up 11 days,  8:49,  1 user,  load average: 1.36, 1.36, 1.04
Tasks: 137 total,   1 running,  79 sleeping,   0 stopped,   0 zombie
%Cpu0  :  6.0 us,  2.7 sy,  0.0 ni,  5.7 id, 84.7 wa,  0.0 hi,  1.0 si,  0.0 st
%Cpu1  :  1.0 us,  3.0 sy,  0.0 ni, 94.7 id,  0.0 wa,  0.0 hi,  1.3 si,  0.0 st
KiB Mem :  8169300 total,  7342244 free,   432912 used,   394144 buff/cache
KiB Swap:        0 total,        0 free,        0 used.  7478748 avail Mem

  PID USER      PR  NI    VIRT    RES    SHR S  %CPU %MEM     TIME+ COMMAND
 9181 root      20   0  193004  27304   8716 S   8.6  0.3   0:07.15 python
 9085 systemd+  20   0   28352   9760   1860 D   5.0  0.1   0:04.34 redis-server
  368 root      20   0       0      0      0 D   1.0  0.0   0:33.88 jbd2/sda1-8
  149 root       0 -20       0      0      0 I   0.3  0.0   0:10.63 kworker/0:1H
 1549 root      20   0  236716  24576   9864 S   0.3  0.3  91:37.30 python3
 
CPU0 的 iowait 比较高
 
// 按下 Ctrl+C，停止 top 命令；然后，执行下面的 iostat 命令
$ iostat -d -x 1
Device            r/s     w/s     rkB/s     wkB/s   rrqm/s   wrqm/s  %rrqm  %wrqm r_await w_await aqu-sz rareq-sz wareq-sz  svctm  %util
...
sda              0.00  492.00      0.00   2672.00     0.00   176.00   0.00  26.35    0.00    1.76   0.00     0.00     5.43   0.00   0.00

磁盘 sda 每秒的写数据（wkB/s）为 2.5MB，I/O 使用率（%util）是 0。虽然有些 I/O 操作，但并没导致磁盘的 I/O 瓶颈

// 终端一中运行下面的 pidstat 命令，观察进程的 I/O 情况
$ pidstat -d 1
12:49:35      UID       PID   kB_rd/s   kB_wr/s kB_ccwr/s iodelay  Command
12:49:36        0       368      0.00     16.00      0.00      86  jbd2/sda1-8
12:49:36      100      9085      0.00    636.00      0.00       1  redis-server

 redis-server 在进行磁盘写
 
// 终端一中，执行 strace 命令，并且指定 redis-server 的进程号 9085
# -f表示跟踪子进程和子线程，-T表示显示系统调用的时长，-tt表示显示跟踪时间
$ strace -f -T -tt -p 9085
[pid  9085] 14:20:16.826131 epoll_pwait(5, [{EPOLLIN, {u32=8, u64=8}}], 10128, 65, NULL, 8) = 1 <0.000055>
[pid  9085] 14:20:16.826301 read(8, "*2\r\n$3\r\nGET\r\n$41\r\nuuid:5b2e76cc-"..., 16384) = 61 <0.000071>
[pid  9085] 14:20:16.826477 read(3, 0x7fff366a5747, 1) = -1 EAGAIN (Resource temporarily unavailable) <0.000063>
[pid  9085] 14:20:16.826645 write(8, "$3\r\nbad\r\n", 9) = 9 <0.000173>
[pid  9085] 14:20:16.826907 epoll_pwait(5, [{EPOLLIN, {u32=8, u64=8}}], 10128, 65, NULL, 8) = 1 <0.000032>
[pid  9085] 14:20:16.827030 read(8, "*2\r\n$3\r\nGET\r\n$41\r\nuuid:55862ada-"..., 16384) = 61 <0.000044>
[pid  9085] 14:20:16.827149 read(3, 0x7fff366a5747, 1) = -1 EAGAIN (Resource temporarily unavailable) <0.000043>
[pid  9085] 14:20:16.827285 write(8, "$3\r\nbad\r\n", 9) = 9 <0.000141>
[pid  9085] 14:20:16.827514 epoll_pwait(5, [{EPOLLIN, {u32=8, u64=8}}], 10128, 64, NULL, 8) = 1 <0.000049>
[pid  9085] 14:20:16.827641 read(8, "*2\r\n$3\r\nGET\r\n$41\r\nuuid:53522908-"..., 16384) = 61 <0.000043>
[pid  9085] 14:20:16.827784 read(3, 0x7fff366a5747, 1) = -1 EAGAIN (Resource temporarily unavailable) <0.000034>
[pid  9085] 14:20:16.827945 write(8, "$4\r\ngood\r\n", 10) = 10 <0.000288>
[pid  9085] 14:20:16.828339 epoll_pwait(5, [{EPOLLIN, {u32=8, u64=8}}], 10128, 63, NULL, 8) = 1 <0.000057>
[pid  9085] 14:20:16.828486 read(8, "*3\r\n$4\r\nSADD\r\n$4\r\ngood\r\n$36\r\n535"..., 16384) = 67 <0.000040>
[pid  9085] 14:20:16.828623 read(3, 0x7fff366a5747, 1) = -1 EAGAIN (Resource temporarily unavailable) <0.000052>
[pid  9085] 14:20:16.828760 write(7, "*3\r\n$4\r\nSADD\r\n$4\r\ngood\r\n$36\r\n535"..., 67) = 67 <0.000060>
[pid  9085] 14:20:16.828970 fdatasync(7) = 0 <0.005415>
[pid  9085] 14:20:16.834493 write(8, ":1\r\n", 4) = 4 <0.000250>

epoll_pwait、read、write、fdatasync 这些系统调用都比较频繁。刚才的写磁盘，应该就是 write 或者 fdatasync 导致

// 接着再来运行 lsof 命令，找出这些系统调用的操作对象
$ lsof -p 9085
redis-ser 9085 systemd-network    3r     FIFO   0,12      0t0 15447970 pipe
redis-ser 9085 systemd-network    4w     FIFO   0,12      0t0 15447970 pipe
redis-ser 9085 systemd-network    5u  a_inode   0,13        0    10179 [eventpoll]
redis-ser 9085 systemd-network    6u     sock    0,9      0t0 15447972 protocol: TCP
redis-ser 9085 systemd-network    7w      REG    8,1  8830146  2838532 /data/appendonly.aof
redis-ser 9085 systemd-network    8u     sock    0,9      0t0 15448709 protocol: TCP

7 号普通文件才会产生磁盘写，文件路径是 /data/appendonly.aof，相应的系统调用包括 write 和 fdatasync

// 用 strace ，观察这个系统调用的执行情况。比如通过 -e 选项指定 fdatasync 
$ strace -f -p 9085 -T -tt -e fdatasync
strace: Process 9085 attached with 4 threads
[pid  9085] 14:22:52.013547 fdatasync(7) = 0 <0.007112>
[pid  9085] 14:22:52.022467 fdatasync(7) = 0 <0.008572>
[pid  9085] 14:22:52.032223 fdatasync(7) = 0 <0.006769>
...
[pid  9085] 14:22:52.139629 fdatasync(7) = 0 <0.008183>

每隔 10ms 左右，就会有一次 fdatasync 调用，并且每次调用本身也要消耗 7~8ms

// 为什么查询时会有磁盘写呢？按理来说不应该只有数据的读取吗？ strace -f -T -tt -p 9085 的结果
read(8, "*2\r\n$3\r\nGET\r\n$41\r\nuuid:53522908-"..., 16384)
write(8, "$4\r\ngood\r\n", 10)
read(8, "*3\r\n$4\r\nSADD\r\n$4\r\ngood\r\n$36\r\n535"..., 16384)
write(7, "*3\r\n$4\r\nSADD\r\n$4\r\ngood\r\n$36\r\n535"..., 67)
write(8, ":1\r\n", 4)

文件描述符编号为 7 的是一个普通文件 /data/appendonly.aof，而编号为 8 的是 TCP socket。8 号对应的 TCP 读写，是一个标准的“请求 - 响应”格式：
从 socket 读取 GET uuid:53522908-… 后，响应 good
再从 socket 读取 SADD good 535… 后，响应 1
观察更多的 strace 结果，每当 GET 返回 good 时，随后都会有一个 SADD 操作，这也就导致了，明明是查询接口，Redis 却有大量的磁盘写


# 由于这两个容器共享同一个网络命名空间，所以我们只需要进入app的网络命名空间即可
$ PID=$(docker inspect --format {{.State.Pid}} app)
# -i表示显示网络套接字信息
$ nsenter --target $PID --net -- lsof -i
COMMAND    PID            USER   FD   TYPE   DEVICE SIZE/OFF NODE NAME
redis-ser 9085 systemd-network    6u  IPv4 15447972      0t0  TCP localhost:6379 (LISTEN)
redis-ser 9085 systemd-network    8u  IPv4 15448709      0t0  TCP localhost:6379->localhost:32996 (ESTABLISHED)
python    9181            root    3u  IPv4 15448677      0t0  TCP *:http (LISTEN)
python    9181            root    5u  IPv4 15449632      0t0  TCP localhost:32996->localhost:6379 (ESTABLISHED)

1. Redis 配置的 appendfsync 是 always，这就导致 Redis 每次的写操作，都会触发 fdatasync 系统调用。今天的案例，没必要用这么高频的同步写，使用默认的 1s 时间间隔，就足够了
2. Python 应用在查询接口中会调用 Redis 的 SADD 命令，这是不合理使用Redis缓存导致，替换为内存即可
```

### 如何迅速分析出系统I/O的瓶颈在哪里？

##### 性能指标

![img](https://static001.geekbang.org/resource/image/9e/38/9e42aaf53ff4a544b9a7b03b6ce63f38.png)

##### 文件系统 I/O 性能指标

* 存储空间的使用情况：包括容量、使用量以及剩余空间等，这些只是文件系统向外展示的空间使用，而非在磁盘空间的真实用量，因为文件系统的元数据也会占用磁盘空间。索引节点的使用情况，它包括容量、使用量以及剩余量等三个指标。如果文件系统中存储过多的小文件，就可能碰到索引节点容量已满的问题
* 缓存使用情况：包括页缓存、目录项缓存、索引节点缓存以及各个具体文件系统（如 ext4、XFS 等）的缓存。这些缓存会使用速度更快的内存，用来临时存储文件数据或者文件系统的元数据，从而可以减少访问慢速磁盘的次数
* 文件 I/O：包括 IOPS（包括 r/s 和 w/s）、响应时间（延迟）以及吞吐量（B/s）等

##### 磁盘 I/O 性能指标

使用率、IOPS（Input/Output Per Second）、吞吐量、响应时间、缓冲区（Buffer）

![img](https://static001.geekbang.org/resource/image/b6/20/b6d67150e471e1340a6f3c3dc3ba0120.png)

##### 性能工具

![img](https://static001.geekbang.org/resource/image/6f/98/6f26fa18a73458764fcda00212006698.png)

![img](https://static001.geekbang.org/resource/image/c4/e9/c48b6664c6d334695ed881d5047446e9.png)

##### 如何迅速分析 I/O 的性能瓶颈

分析思路：

* 先用 iostat 发现磁盘 I/O 性能瓶颈
* 再借助 pidstat ，定位出导致瓶颈的进程
* 随后分析进程的 I/O 行为
* 结合应用程序的原理，分析这些 I/O 的来源

为了缩小排查范围，通常会先运行那几个支持指标较多的工具，如 iostat、vmstat、pidstat 等。然后再根据观察到的现象，结合系统和应用程序的原理，寻找下一步的分析方向

![img](https://static001.geekbang.org/resource/image/18/8a/1802a35475ee2755fb45aec55ed2d98a.png)

### 磁盘 I/O 性能优化的几个思路

##### I/O 基准测试

分析工具：

* fio（Flexible I/O Tester）：最常用的文件系统和磁盘 I/O 性能基准测试工具。它提供了大量的可定制化选项，可以用来测试，裸盘或者文件系统在各种场景下的 I/O 性能，包括了不同块大小、不同 I/O 引擎以及是否使用缓存等场景

用磁盘路径测试写，会破坏这个磁盘中的文件系统，所以在使用前，一定要事先做好数据备份

```
// 安装
apt-get install -y fio

// 测试
# 随机读
fio -name=randread -direct=1 -iodepth=64 -rw=randread -ioengine=libaio -bs=4k -size=1G -numjobs=1 -runtime=1000 -group_reporting -filename=/dev/sdb

# 随机写
fio -name=randwrite -direct=1 -iodepth=64 -rw=randwrite -ioengine=libaio -bs=4k -size=1G -numjobs=1 -runtime=1000 -group_reporting -filename=/dev/sdb

# 顺序读
fio -name=read -direct=1 -iodepth=64 -rw=read -ioengine=libaio -bs=4k -size=1G -numjobs=1 -runtime=1000 -group_reporting -filename=/dev/sdb

# 顺序写
fio -name=write -direct=1 -iodepth=64 -rw=write -ioengine=libaio -bs=4k -size=1G -numjobs=1 -runtime=1000 -group_reporting -filename=/dev/sdb 

direct：是否跳过系统缓存。上面示例中，1 就表示跳过系统缓存
iodepth：是否使用异步 I/O（asynchronous I/O，简称 AIO）时，同时发出的 I/O 请求上限。设置的是 64
rw：I/O 模式。 read/write 分别表示顺序读 / 写，而 randread/randwrite 则分别表示随机读 / 写
ioengine： I/O 引擎，它支持同步（sync）、异步（libaio）、内存映射（mmap）、网络（net）等各种 I/O 引擎
bs：I/O 的大小。设置成了 4K（这也是默认值）
filename：文件路径

// 顺序读的测试报告
read: (g=0): rw=read, bs=(R) 4096B-4096B, (W) 4096B-4096B, (T) 4096B-4096B, ioengine=libaio, iodepth=64
fio-3.1
Starting 1 process
Jobs: 1 (f=1): [R(1)][100.0%][r=16.7MiB/s,w=0KiB/s][r=4280,w=0 IOPS][eta 00m:00s]
read: (groupid=0, jobs=1): err= 0: pid=17966: Sun Dec 30 08:31:48 2018
   read: IOPS=4257, BW=16.6MiB/s (17.4MB/s)(1024MiB/61568msec)
    slat (usec): min=2, max=2566, avg= 4.29, stdev=21.76
    clat (usec): min=228, max=407360, avg=15024.30, stdev=20524.39
     lat (usec): min=243, max=407363, avg=15029.12, stdev=20524.26
    clat percentiles (usec):
     |  1.00th=[   498],  5.00th=[  1020], 10.00th=[  1319], 20.00th=[  1713],
     | 30.00th=[  1991], 40.00th=[  2212], 50.00th=[  2540], 60.00th=[  2933],
     | 70.00th=[  5407], 80.00th=[ 44303], 90.00th=[ 45351], 95.00th=[ 45876],
     | 99.00th=[ 46924], 99.50th=[ 46924], 99.90th=[ 48497], 99.95th=[ 49021],
     | 99.99th=[404751]
   bw (  KiB/s): min= 8208, max=18832, per=99.85%, avg=17005.35, stdev=998.94, samples=123
   iops        : min= 2052, max= 4708, avg=4251.30, stdev=249.74, samples=123
  lat (usec)   : 250=0.01%, 500=1.03%, 750=1.69%, 1000=2.07%
  lat (msec)   : 2=25.64%, 4=37.58%, 10=2.08%, 20=0.02%, 50=29.86%
  lat (msec)   : 100=0.01%, 500=0.02%
  cpu          : usr=1.02%, sys=2.97%, ctx=33312, majf=0, minf=75
  IO depths    : 1=0.1%, 2=0.1%, 4=0.1%, 8=0.1%, 16=0.1%, 32=0.1%, >=64=100.0%
     submit    : 0=0.0%, 4=100.0%, 8=0.0%, 16=0.0%, 32=0.0%, 64=0.0%, >=64=0.0%
     complete  : 0=0.0%, 4=100.0%, 8=0.0%, 16=0.0%, 32=0.0%, 64=0.1%, >=64=0.0%
     issued rwt: total=262144,0,0, short=0,0,0, dropped=0,0,0
     latency   : target=0, window=0, percentile=100.00%, depth=64

Run status group 0 (all jobs):
   READ: bw=16.6MiB/s (17.4MB/s), 16.6MiB/s-16.6MiB/s (17.4MB/s-17.4MB/s), io=1024MiB (1074MB), run=61568-61568msec

Disk stats (read/write):
  sdb: ios=261897/0, merge=0/0, ticks=3912108/0, in_queue=3474336, util=90.09% 
  
slat：从 I/O 提交到实际执行 I/O 的时长（Submission latency），由于 I/O 提交和 I/O 完成是一个动作，所以 slat 实际上就是 I/O 完成的时间，而 clat 是 0
clat：从 I/O 提交到 I/O 完成的时长（Completion latency）
lat： 从 fio 创建 I/O 到 I/O 完成的总时长，使用异步 I/O（libaio）时，lat 近似等于 slat + clat 之和

// fio 支持 I/O 的重放。借助前面提到过的 blktrace，再配合上 fio，就可以实现对应用程序 I/O 模式的基准测试。你需要先用 blktrace ，记录磁盘设备的 I/O 访问情况；然后使用 fio ，重放 blktrace 的记录

# 使用blktrace跟踪磁盘I/O，注意指定应用程序正在操作的磁盘
$ blktrace /dev/sdb

# 查看blktrace记录的结果
# ls
sdb.blktrace.0  sdb.blktrace.1

# 将结果转化为二进制文件
$ blkparse sdb -d sdb.bin

# 使用fio重放日志
$ fio --name=replay --filename=/dev/sdb --direct=1 --read_iolog=sdb.bin 
```

##### I/O 性能优化

得到 I/O 基准测试报告后，再用性能分析套路，找出 I/O 的性能瓶颈并优化，就水到渠成了

##### 应用程序优化

* 第一，可以用追加写代替随机写，减少寻址开销，加快 I/O 写的速度
* 第二，可以借助缓存 I/O ，充分利用系统缓存，降低实际 I/O 的次数
* 第三，可以在应用程序内部构建自己的缓存，或者用 Redis 这类外部缓存系统，C 标准库提供的 fopen、fread 等库函数，都会利用标准库的缓存，减少磁盘的操作。而你直接使用 open、read 等系统调用时，就只能利用操作系统提供的页缓存和缓冲区等，而没有库函数的缓存可用
* 第四，在需要频繁读写同一块磁盘空间时，可以用 mmap 代替 read/write，减少内存的拷贝次数
* 第五，在需要同步写的场景中，尽量将写请求合并，而不是让每个请求都同步写入磁盘，即可以用 fsync() 取代 O_SYNC
* 第六，在多个应用程序共享相同磁盘时，为了保证 I/O 不被某个应用完全占用，推荐使用 cgroups 的 I/O 子系统，来限制进程 / 进程组的 IOPS 以及吞吐量
* 在使用 CFQ 调度器时，可以用 ionice 来调整进程的 I/O 调度优先级，特别是提高核心应用的 I/O 优先级

##### 文件系统优化

* 第一，根据实际负载场景的不同，选择最适合的文件系统。比如 Ubuntu 默认使用 ext4 文件系统，而 CentOS 7 默认使用 xfs 文件系统
* 第二，进一步优化文件系统的配置选项，包括文件系统的特性（如 ext_attr、dir_index）、日志模式（如 journal、ordered、writeback）、挂载选项（如 noatime）等。如用 tune2fs 工具，可以调整文件系统的特性（tune2fs 也常用来查看文件系统超级块的内容）。 通过 /etc/fstab ，或者 mount 命令行参数，可以调整文件系统的日志模式和挂载选项等
* 第三，优化文件系统的缓存。如优化 pdflush 脏页的刷新频率（设置 dirty_expire_centisecs 和 dirty_writeback_centisecs）以及脏页的限额（调整 dirty_background_ratio 和 dirty_ratio 等）。还可以优化内核回收目录项缓存和索引节点缓存的倾向，即调整 vfs_cache_pressure（/proc/sys/vm/vfs_cache_pressure，默认值 100），数值越大，就表示越容易回收
* 第四，在不需要持久化时，可以用内存文件系统  tmpfs，以获得更好的 I/O 性能 。tmpfs 把数据直接保存在内存中，而不是磁盘中。比如 /dev/shm/ ，就是大多数 Linux 默认配置的一个内存文件系统，它的大小默认为总内存的一半

##### 磁盘优化

* 第一，换用性能更好的磁盘，比如用 SSD 替代 HDD
* 第二，使用 RAID ，把多块磁盘组合成一个逻辑磁盘，构成冗余独立磁盘阵列。既可以提高数据的可靠性，又可以提升数据的访问性能
* 第三，针对磁盘和应用程序 I/O 模式的特征，选择最适合的 I/O 调度算法。SSD 和虚拟机中的磁盘，通常用的是 noop 调度算法。而数据库应用，更推荐使用 deadline 算法
* 第四，对应用程序的数据，进行磁盘级别的隔离。比如，可以为日志、数据库等 I/O 压力比较重的应用，配置单独的磁盘
* 第五，在顺序读比较多的场景中，可以增大磁盘的预读数据，比如，通过下面两种方法，调整 /dev/sdb 的预读大小
  * 调整内核选项 /sys/block/sdb/queue/read_ahead_kb，默认大小是 128 KB，单位为 KB
  * 使用 blockdev 工具设置，比如 blockdev --setra 8192 /dev/sdb，注意这里的单位是 512B（0.5KB），所以它的数值总是 read_ahead_kb 的两倍

* 第六，优化内核块设备 I/O 的选项。如调整磁盘队列的长度 /sys/block/sdb/queue/nr_requests，适当增大队列长度，可以提升磁盘的吞吐量（当然也会导致 I/O 延迟增大）

* 第七，磁盘本身出现硬件错误，也会导致 I/O 性能急剧下降，所以发现磁盘性能急剧下降时，可以查看 dmesg 中是否有硬件 I/O 故障的日志。  还可以使用 badblocks、smartctl 等工具，检测磁盘的硬件问题，或用 e2fsck 等来检测文件系统的错误。如果发现问题，可以使用 fsck 等工具来修复