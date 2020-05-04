# RAM

### Linux内存是怎么工作的？

##### 内存映射

Linux 内核给每个进程都提供了一个独立的虚拟地址空间，并且这个地址空间是连续的。这样进程就可以很方便地访问内存，更确切地说是访问虚拟内存。内存映射，就是将虚拟内存地址映射到物理内存地址。内核为每个进程都维护了一张页表，记录虚拟地址与物理地址的映射关系

![img](https://static001.geekbang.org/resource/image/fc/b6/fcfbe2f8eb7c6090d82bf93ecdc1f0b6.png)

页表实际上存储在 CPU 的内存管理单元 MMU 中，这样，正常情况下，处理器就可以直接通过硬件，找出要访问的内存。而当进程访问的虚拟地址在页表中查不到时，系统会产生一个缺页异常，进入内核空间分配物理内存、更新进程页表，最后再返回用户空间，恢复进程的运行。另外，TLB（Translation Lookaside Buffer，转译后备缓冲器）会影响 CPU 的内存访问性能。TLB 其实就是 MMU 中页表的高速缓存。由于进程的虚拟地址空间是独立的，而 TLB 的访问速度又比 MMU 快得多，所以，通过减少进程的上下文切换，减少 TLB 的刷新次数，就可以提高 TLB 缓存的使用率，进而提高 CPU 的内存访问性能。

MMU 并不以字节为单位来管理内存，而是页，通常是 4 KB 大小。这样，每一次内存映射，都需要关联 4 KB 或者 4KB 整数倍的内存空间。页的大小只有 4 KB ，导致整个页表会变得非常大。仅 32 位系统就需要 100 多万个页表项（4GB/4KB），才可以实现整个地址空间的映射。为了解决页表项过多的问题，Linux 提供了两种机制

![img](https://static001.geekbang.org/resource/image/b5/25/b5c9179ac64eb5c7ca26448065728325.png)

* 多级页表：Linux 用的正是四级页表来管理内存页，如图所示，虚拟地址被分为 5 个部分，前 4 个表项用于选择页，而最后一个索引表示页内偏移
* 大页（HugePage）：就是比普通页更大的内存块，常见的大小有 2MB 和 1GB。大页通常用在使用大量内存的进程上，比如 Oracle、DPDK 等

##### 虚拟内存空间分布

![img](https://static001.geekbang.org/resource/image/71/5d/71a754523386cc75f4456a5eabc93c5d.png)

* 只读段：包括代码和常量等
* 数据段：包括全局变量等
* 堆：包括动态分配的内存，从低地址开始向上增长
* 文件映射段：包括动态库、共享内存等，从高地址开始向下增长
* 栈：包括局部变量和函数调用的上下文等，栈的大小是固定的，一般是 8 MB

堆和文件映射段的内存是动态分配的，如 C 标准库的 malloc() 或者 mmap() 

##### 内存分配与回收

malloc() 是 C 标准库提供的内存分配函数，对应到系统调用上，有两种实现方式，即 brk() 和 mmap()

* brk：小块内存（小于 128K），C 标准库使用 brk() 来分配，也就是通过移动堆顶的位置来分配内存。这些内存释放后并不会立刻归还系统，而是被缓存起来，这样就可以重复使用。可以减少缺页异常的发生，提高内存访问效率。由于这些内存没有归还系统，在内存工作繁忙时，频繁的内存分配和释放会造成内存碎片。由于brk分配的内存是推_edata指针，从堆的低地址向高地址推进。这种情况下，如果高地址的内存不释放，低地址的内存是得不到释放的
* mmap：大块内存（大于 128K），则直接使用内存映射 mmap() 来分配，也就是在文件映射段找一块空闲内存分配出去。会在释放时直接归还系统，所以每次 mmap 都会发生缺页异常。在内存工作繁忙时，频繁的内存分配会导致大量的缺页异常，使内核的管理负担增大。这也是 malloc 只对大块内存使用 mmap  的原因

当这两种调用发生后，其实并没有真正分配内存。这些内存，都只在首次访问时才分配，也就是通过缺页异常进入内核中，再由内核来分配内存。应用程序用完内存后，还需要调用 free() 或 unmap() ，来释放这些不用的内存

用户空间的内存分配都是基于buddy算法（伙伴算法）。只有在内核空间，内核调用kmalloc去分配内存的时候，才会涉及到slab，Linux 则通过 slab 分配器来管理不到 1K小内存

系统也不会任由某个进程用完所有内存。在发现内存紧张时，系统就会通过一系列机制来回收内存

* 回收缓存，比如使用 LRU（Least Recently Used）算法，回收最近使用最少的内存页面
* 回收不常访问的内存：把不常用的内存通过交换分区直接写到磁盘中，Swap 其实就是把一块磁盘空间当成内存来用。它可以把进程暂时不用的数据存储到磁盘中（这个过程称为换出），当进程访问这些内存时，再从磁盘读取这些数据到内存中（这个过程称为换入）。通常只在内存不足时，才会发生 Swap 交换。并且由于磁盘读写的速度远比内存慢，Swap 会导致严重的内存性能问题
* 杀死进程：内存紧张时系统还会通过 OOM（Out of Memory），直接杀掉占用大量内存的进程。一个进程消耗的内存越大，oom_score 就越大；一个进程运行占用的 CPU 越多，oom_score 就越小。进程的 oom_score 越大，代表消耗的内存越多，也就越容易被 OOM 杀死

```
// oom_adj 的范围是 [-17, 15]，数值越大，表示进程越容易被 OOM 杀死；其中 -17 表示禁止 OOM
// 把 sshd 进程的 oom_adj 调小为 -16，这样， sshd 进程就不容易被 OOM 杀死
echo -16 > /proc/$(pidof sshd)/oom_adj
```

##### 查看内存使用情况

分析工具：

* free：显示整个系统的内存使用情况

```
// 注意不同版本的free输出可能会有所不同，默认以字节为单位
free
              total        used        free      shared  buff/cache   available
Mem:        8169348      263524     6875352         668     1030472     7611064
Swap:             0           0           0
```

* 第一列，total 是总内存大小
* 第二列，used 是已使用内存的大小，包含了共享内存
* 第三列，free 是未使用内存的大小
* 第四列，shared 是共享内存的大小
* 第五列，buff/cache 是缓存和缓冲区的大小
* 最后一列，available 是新进程可用内存的大小，不仅包含未使用内存，还包括了可回收的缓存，所以一般会比未使用内存更大

可以用 top 或者 ps 等工具查看进程的内存使用情况

```
//  按下M切换到内存排序
top
...
KiB Mem :  8169348 total,  6871440 free,   267096 used,  1030812 buff/cache
KiB Swap:        0 total,        0 free,        0 used.  7607492 avail Mem

  PID USER      PR  NI    VIRT    RES    SHR S  %CPU %MEM     TIME+ COMMAND
  430 root      19  -1  122360  35588  23748 S   0.0  0.4   0:32.17 systemd-journal
 1075 root      20   0  771860  22744  11368 S   0.0  0.3   0:38.89 snapd
 1048 root      20   0  170904  17292   9488 S   0.0  0.2   0:00.24 networkd-dispat
    1 root      20   0   78020   9156   6644 S   0.0  0.1   0:22.92 systemd
12376 azure     20   0   76632   7456   6420 S   0.0  0.1   0:00.01 systemd
12374 root      20   0  107984   7312   6304 S   0.0  0.1   0:00.00 sshd
...
```

* VIRT 是进程虚拟内存的大小，只要是进程申请过的内存，即便还没有真正分配物理内存，也会计算在内
* RES 是常驻内存的大小，也就是进程实际使用的物理内存大小，但不包括 Swap 和共享内存
* SHR 是共享内存的大小，比如与其他进程共同使用的共享内存、加载的动态链接库以及程序的代码段等
* %MEM 是进程使用物理内存占系统总内存的百分比

虚拟内存通常并不会全部分配物理内存。每个进程的虚拟内存都比常驻内存大得多。共享内存 SHR 并不一定是共享的，如程序的代码段、非共享的动态链接库，也都算在 SHR 里。SHR 也包括了进程间真正共享的内存

### 怎么理解内存中的Buffer和Cache？

磁盘和文件这两种读写方式所使用的缓存是不同的，也就是 Cache 和 Buffer 区别

* 磁盘：是一个块设备，可以划分为不同的分区；在分区之上再创建文件系统，挂载到某个目录，之后才可以在这个目录中读写文件。读写磁盘或者分区时，就会跳过文件系统，也就是所谓的“裸I/O“

* 文件：Linux 中“一切皆文件”，指的是所有类型的文件，一般而言则是普通文件。在读写普通文件时，会经过文件系统，由文件系统负责与磁盘交互

```
// 注意不同版本的free输出可能会有所不同
free
              total        used        free      shared  buff/cache   available
Mem:        8169348      263524     6875352         668     1030472     7611064
Swap:             0           0           0
```

##### free 数据的来源

* Buffers 是内核缓冲区用到的内存，对应的是  /proc/meminfo 中的 Buffers 值。是对原始磁盘块的临时存储，也就是用来缓存磁盘的数据，通常不会特别大（20MB 左右）。这样，内核就可以把分散的写集中起来，统一优化磁盘的写入，比如可以把多次小的写合并成单次大的写等等
* Cache 是内核页缓存和 Slab 用到的内存，对应的是  /proc/meminfo 中的 Cached 与 SReclaimable 之和。是从磁盘读取文件的页缓存，也就是用来缓存从文件读取的数据。这样，下次访问这些文件数据时，就可以直接从内存中快速获取，而不需要再次访问缓慢的磁盘。SReclaimable 是 Slab 的一部分。Slab 包括两部分，其中的可回收部分，用 SReclaimable 记录；而不可回收部分，用 SUnreclaim 记录

##### 案例分析

```
// 清理文件页、目录项、Inodes等各种缓存
$ echo 3 > /proc/sys/vm/drop_caches
```

磁盘和文件写的情况：

```
// 第一个终端每隔1秒输出1组数据
$ vmstat 1
procs -----------memory---------- ---swap-- -----io---- -system-- ------cpu-----
r  b   swpd   free   buff  cache   si   so    bi    bo   in   cs us sy id wa st
0  0      0 7743608   1112  92168    0    0     0     0   52  152  0  1 100  0  0
0  0      0 7743608   1112  92168    0    0     0     0   36   92  0  0 100  0  0

buff 和 cache 就是free里面的 Buffers 和 Cache，单位是 KB
bi 和 bo 分别表示块设备读取和写入的大小，单位为块 / 秒。Linux 中块的大小是 1KB，所以这个单位也是 KB/s

// 通过读取随机设备，生成一个 500MB 大小的文件
$ dd if=/dev/urandom of=/tmp/file bs=1M count=500

// 观察第一个终端的情况
procs -----------memory---------- ---swap-- -----io---- -system-- ------cpu-----
r  b   swpd   free   buff  cache   si   so    bi    bo   in   cs us sy id wa st
0  0      0 7499460   1344 230484    0    0     0     0   29  145  0  0 100  0  0
 1  0      0 7338088   1752 390512    0    0   488     0   39  558  0 47 53  0  0
 1  0      0 7158872   1752 568800    0    0     0     4   30  376  1 50 49  0  0
 1  0      0 6980308   1752 747860    0    0     0     0   24  360  0 50 50  0  0
 0  0      0 6977448   1752 752072    0    0     0     0   29  138  0  0 100  0  0
 0  0      0 6977440   1760 752080    0    0     0   152   42  212  0  1 99  1  0
...
 0  1      0 6977216   1768 752104    0    0     4 122880   33  234  0  1 51 49  0
 0  1      0 6977440   1768 752108    0    0     0 10240   38  196  0  0 50 50  0
 
Cache 在不停地增长，而 Buffer 基本保持不变
 
// 第二个终端继续运行dd命令向磁盘分区/dev/sdb1写入2G数据
// 首先清理缓存
$ echo 3 > /proc/sys/vm/drop_caches
$ dd if=/dev/urandom of=/dev/sdb1 bs=1M count=2048 

// 观察第一个终端的情况
procs -----------memory---------- ---swap-- -----io---- -system-- ------cpu-----
 r  b   swpd   free   buff  cache   si   so    bi    bo   in   cs us sy id wa st
 1  0      0 7584780 153592  97436    0    0   684     0   31  423  1 48 50  2  0
 1  0      0 7418580 315384 101668    0    0     0     0   32  144  0 50 50  0  0
 1  0      0 7253664 475844 106208    0    0     0     0   20  137  0 50 50  0  0
 1  0      0 7093352 631800 110520    0    0     0     0   23  223  0 50 50  0  0
 1  1      0 6930056 790520 114980    0    0     0 12804   23  168  0 50 42  9  0
 1  0      0 6757204 949240 119396    0    0     0 183804   24  191  0 53 26 21  0
 1  1      0 6591516 1107960 123840    0    0     0 77316   22  232  0 52 16 33  0

写磁盘时（也就是 bo 大于  0 时），Buffer 和 Cache 都在增长，但显然 Buffer 的增长快得多
```

磁盘和文件读的情况：

```
// 第二个终端从文件 /tmp/file 中，读取数据写入空设备
// 首先清理缓存
$ echo 3 > /proc/sys/vm/drop_caches
$ dd if=/tmp/file of=/dev/null

// 观察第一个终端的情况
procs -----------memory---------- ---swap-- -----io---- -system-- ------cpu-----
 r  b   swpd   free   buff  cache   si   so    bi    bo   in   cs us sy id wa st
 0  1      0 7724164   2380 110844    0    0 16576     0   62  360  2  2 76 21  0
 0  1      0 7691544   2380 143472    0    0 32640     0   46  439  1  3 50 46  0
 0  1      0 7658736   2380 176204    0    0 32640     0   54  407  1  4 50 46  0
 0  1      0 7626052   2380 208908    0    0 32640    40   44  422  2  2 50 46  0
 
 Buffer 保持不变，而 Cache 则在不停增长
 
// 从磁盘分区 /dev/sda1 中读取数据，写入空设备 
// 首先清理缓存
$ echo 3 > /proc/sys/vm/drop_caches
# 运行dd命令读取文件
$ dd if=/dev/sda1 of=/dev/null bs=1M count=1024

// 观察第一个终端的情况

procs -----------memory---------- ---swap-- -----io---- -system-- ------cpu-----
 r  b   swpd   free   buff  cache   si   so    bi    bo   in   cs us sy id wa st
 0  0      0 7225880   2716 608184    0    0     0     0   48  159  0  0 100  0  0
 0  1      0 7199420  28644 608228    0    0 25928     0   60  252  0  1 65 35  0
 0  1      0 7167092  60900 608312    0    0 32256     0   54  269  0  1 50 49  0
 0  1      0 7134416  93572 608376    0    0 32672     0   53  253  0  0 51 49  0
 0  1      0 7101484 126320 608480    0    0 32748     0   80  414  0  1 50 49  0
 
 Buffer 和 Cache 都在增长，但显然 Buffer 的增长快很多
```

Buffer 是对磁盘数据的缓存，而 Cache 是文件数据的缓存，它们既会用在读请求中，也会用在写请求中

### 如何利用系统缓存优化程序的运行效率？

Buffers 和 Cache 可以极大提升系统的 I/O 性能。通常，用缓存命中率，来衡量缓存的使用效率。命中率越高，表示缓存被利用得越充分，应用程序的性能也就越好。Buffers 和 Cache 都是操作系统来管理的，应用程序并不能直接控制这些缓存的内容和生命周期。在应用程序开发中，一般要用专门的缓存组件，来进一步提升性能。比如，程序内部可以使用堆或者栈明确声明内存空间，来存储需要缓存的数据。再或者，使用 Redis 这类外部缓存服务，优化数据的访问效率

##### 缓存命中率

缓存的命中率：是指直接通过缓存获取数据的请求次数，占所有数据请求次数的百分比。命中率越高，表示使用缓存带来的收益越高，应用程序的性能也就越好。缓存是现在所有高并发系统必需的核心模块，主要作用就是把经常访问的数据（也就是热点数据），提前读入到内存中。下次访问时就可以直接从内存读取数据，而不需要经过硬盘，从而加快应用程序的响应速度。

分析工具：

* cachestat 和 cachetop：Linux 系统中并没有直接提供缓存命中率的接口，这两个正是查看系统缓存命中情况的工具。cachestat 提供了整个操作系统缓存的读写命中情况。cachetop 提供了每个进程的缓存命中情况，它们都是BBC软件包的一部分

```
// 安装BBC软件包，bcc-tools 需要内核版本为 4.1 或者更新的版本
sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 4052245BD4284CDD
echo "deb https://repo.iovisor.org/apt/xenial xenial main" | sudo tee /etc/apt/sources.list.d/iovisor.list
sudo apt-get update
sudo apt-get install -y bcc-tools libbcc-examples linux-headers-$(uname -r)

// 设置环境变量
export PATH=$PATH:/usr/share/bcc/tools
```

```
// 以 1 秒的时间间隔，输出了 3 组缓存统计数据
$ cachestat 1 3
   TOTAL   MISSES     HITS  DIRTIES   BUFFERS_MB  CACHED_MB
       2        0        2        1           17        279
       2        0        2        1           17        279
       2        0        2        1           17        279 
       
TOTAL ，表示总的 I/O 次数
MISSES ，表示缓存未命中的次数
HITS ，表示缓存命中的次数
DIRTIES， 表示新增到缓存中的脏页数
BUFFERS_MB 表示 Buffers 的大小，以 MB 为单位
CACHED_MB 表示 Cache 的大小，以 MB 为单位     


$ cachetop
11:58:50 Buffers MB: 258 / Cached MB: 347 / Sort: HITS / Order: ascending
PID      UID      CMD              HITS     MISSES   DIRTIES  READ_HIT%  WRITE_HIT%
   13029 root     python            1        0        0        100.0%       0.0%
   
READ_HIT 和 WRITE_HIT ，分别表示读和写的缓存命中率   
```

##### 指定文件的缓存大小

分析工具：

* pcstat：是一个基于 Go 语言开发的工具，所以安装它之前，首先应该安装 Go 语言

```
// 安装完 Go 语言(https://golang.org/dl/)，再运行下面的命令安装 pcstat
$ export GOPATH=~/go
$ export PATH=~/go/bin:$PATH
$ go get golang.org/x/sys/unix
$ go get github.com/tobert/pcstat/pcstat
```

```
// 展示了 /bin/ls 这个文件的缓存情况
$ pcstat /bin/ls
+---------+----------------+------------+-----------+---------+
| Name    | Size (bytes)   | Pages      | Cached    | Percent |
|---------+----------------+------------+-----------+---------|
| /bin/ls | 133792         | 33         | 0         | 000.000 |
+---------+----------------+------------+-----------+---------+
执行一下 ls 命令，再运行相同的命令来查看的话，就会发现 /bin/ls 都在缓存中了
```

##### 案例分析

```
// 使用 dd 命令生成一个512MB的临时文件，用于后面的文件读取测试
$ dd if=/dev/sda1 of=file bs=1M count=512
# 清理缓存
$ echo 3 > /proc/sys/vm/drop_caches

// 第一个终端
$ pcstat file
+-------+----------------+------------+-----------+---------+
| Name  | Size (bytes)   | Pages      | Cached    | Percent |
|-------+----------------+------------+-----------+---------|
| file  | 536870912      | 131072     | 0         | 000.000 |
+-------+----------------+------------+-----------+---------+

// 第一个终端 每隔5秒刷新一次数据
$ cachetop 5

// 第二个终端，运行 dd 命令测试文件的读取速度
$ dd if=file of=/dev/null bs=1M
512+0 records in
512+0 records out
536870912 bytes (537 MB, 512 MiB) copied, 16.0509 s, 33.4 MB/s

读性能是 33.4 MB/s。在 dd 命令运行前已经清理了缓存，所以 dd 命令读取数据时，肯定要通过文件系统从磁盘中读取

// 第一个终端， 查看 cachetop 界面的缓存命中情况
PID      UID      CMD              HITS     MISSES   DIRTIES  READ_HIT%  WRITE_HIT%
\.\.\.
    3264 root     dd                37077    37330        0      49.8%      50.2%
    
请求的缓存命中率只有 50% 

// 第二个终端，运行 dd 命令测试文件的读取速度
$ dd if=file of=/dev/null bs=1M
512+0 records in
512+0 records out
536870912 bytes (537 MB, 512 MiB) copied, 0.118415 s, 4.5 GB/s

磁盘的读性能居然变成了 4.5 GB/s，比第一次的结果明显高了太多

// 第一个终端， 查看 cachetop 界面的缓存命中情况
10:45:22 Buffers MB: 4 / Cached MB: 719 / Sort: HITS / Order: ascending
PID      UID      CMD              HITS     MISSES   DIRTIES  READ_HIT%  WRITE_HIT%
\.\.\.
   32642 root     dd               131637        0        0     100.0%       0.0%
   
读的缓存命中率是 100.0%，也就是说这次的 dd 命令全部命中了缓存，所以才会看到那么高的性能

// 第二个终端，再次执行 pcstat 查看文件 file 的缓存情况
$ pcstat file
+-------+----------------+------------+-----------+---------+
| Name  | Size (bytes)   | Pages      | Cached    | Percent |
|-------+----------------+------------+-----------+---------|
| file  | 536870912      | 131072     | 131072    | 100.000 |
+-------+----------------+------------+-----------+---------+

第二次读取文件的时候，测试文件 file 已经被全部缓存了起来，这跟刚才观察到的缓存命中率 100% 是一致的
```

```
// 第一个终端，每隔5秒刷新一次数据
$ cachetop 5 

// 第二个终端，模拟从磁盘读取32M数据
$ docker run --privileged --name=app -itd feisky/app:io-direct
$ docker logs app
Reading data from disk /dev/sdb1 with buffer size 33554432
Time used: 0.929935 s to read 33554432 bytes
Time used: 0.949625 s to read 33554432 bytes

读取32M数据花了0.9秒时间，太慢了

// 第一个终端
16:39:18 Buffers MB: 73 / Cached MB: 281 / Sort: HITS / Order: ascending
PID      UID      CMD              HITS     MISSES   DIRTIES  READ_HIT%  WRITE_HIT%
   21881 root     app               1024        0        0     100.0%       0.0% 

每秒实际读取的数据大小。HITS 代表缓存的命中次数，那么每次命中能读取多少数据呢？自然是一页。内存以页为单位进行管理，而每个页的大小是 4KB。所以，在 5 秒的时间间隔里，命中的缓存为 1024*4K/1024 = 4MB，再除以 5 秒，可以得到每秒读的缓存是 0.8MB，显然跟案例应用的 32 MB/s 相差太多
如果为系统调用设置直接 I/O 的标志，就可以绕过系统缓存。要判断应用程序是否用了直接 I/O，最简单的方法当然是用strace观察它的系统调用，查找应用程序在调用它们时的选项

// pgrep 命令来查找案例进程的 PID 号
# strace -p $(pgrep app)
strace: Process 4988 attached
restart_syscall(<\.\.\. resuming interrupted nanosleep \.\.\.>) = 0
openat(AT_FDCWD, "/dev/sdb1", O_RDONLY|O_DIRECT) = 4
mmap(NULL, 33558528, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x7f448d240000
read(4, "8vq\213\314\264u\373\4\336K\224\25@\371\1\252\2\262\252q\221\n0\30\225bD\252\266@J"\.\.\., 33554432) = 33554432
write(1, "Time used: 0.948897 s to read 33"\.\.\., 45) = 45
close(4)                                = 0

应用调用了 openat 来打开磁盘分区 /dev/sdb1，并且传入的参数为 O_RDONLY|O_DIRECT
如果去掉代码中的标志 O_DIRECT

$ docker logs app
Reading data from disk /dev/sdb1 with buffer size 33554432
Time used: 0.037342 s s to read 33554432 bytes
Time used: 0.029676 s to read 33554432 bytes

0.03 秒，就可以读取 32MB 数据，明显比之前的 0.9 秒快多了。这次应该用了系统缓存

// 第一个终端，查看 cachetop 的输出来确认一下
16:40:08 Buffers MB: 73 / Cached MB: 281 / Sort: HITS / Order: ascending
PID      UID      CMD              HITS     MISSES   DIRTIES  READ_HIT%  WRITE_HIT%
   22106 root     app               40960        0        0     100.0%       0.0%
   
读的命中率还是 100%，HITS （即命中数）变成了 40960，换算成每秒字节数正好是 32 MB（即 40960*4k/5/1024=32M）   
优化前，通过 cachetop 只能看到很少一部分数据的全部命中，而没有观察到大量数据的未命中情况是因为，cachetop 工具并不把直接 I/O 算进来
```

### 内存泄漏了，该如何定位和处理？

实际应用程序可能会比较复杂

* malloc() 和 free() 通常并不是成对出现，而是需要在每个异常处理路径和成功路径上都释放内存 
* 在多线程程序中，一个线程中分配的内存，可能会在另一个线程中访问和释放
* 在第三方的库函数中，隐式分配的内存可能需要应用程序显式释放

##### 案例分析

分析工具：

* sysstat 软件包中的 vmstat ，可以观察内存的变化情况
* memleak 可以跟踪系统或指定进程的内存分配、释放请求，然后定期输出一个未释放内存和相应调用栈的汇总情况，是bbc包中的一个工具

预先安装 sysstat、Docker 以及 bcc 软件包

```
# install sysstat docker
sudo apt-get install -y sysstat docker.io

# Install bcc
sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 4052245BD4284CDD
echo "deb https://repo.iovisor.org/apt/bionic bionic main" | sudo tee /etc/apt/sources.list.d/iovisor.list
sudo apt-get update
sudo apt-get install -y bcc-tools libbcc-examples linux-headers-$(uname -r)
```

```
// 第一个终端运行斐波那契数列的程序，一秒打印一次
$ docker run --name=app -itd feisky/app:mem-leak
$ docker logs app
2th => 1
3th => 2
4th => 3
5th => 5
6th => 8
7th => 13

// 第二个终端运行下面的 vmstat ，等待一段时间，观察内存的变化情况，每隔3秒输出一组数据
$ vmstat 3
procs -----------memory---------- ---swap-- -----io---- -system-- ------cpu-----
r  b   swpd   free   buff  cache   si   so    bi    bo   in   cs us sy id wa st
procs -----------memory---------- ---swap-- -----io---- -system-- ------cpu-----
r  b   swpd   free   buff  cache   si   so    bi    bo   in   cs us sy id wa st
0  0      0 6601824  97620 1098784    0    0     0     0   62  322  0  0 100  0  0
0  0      0 6601700  97620 1098788    0    0     0     0   57  251  0  0 100  0  0
0  0      0 6601320  97620 1098788    0    0     0     3   52  306  0  0 100  0  0
0  0      0 6601452  97628 1098788    0    0     0    27   63  326  0  0 100  0  0
2  0      0 6601328  97628 1098788    0    0     0    44   52  299  0  0 100  0  0
0  0      0 6601080  97628 1098792    0    0     0     0   56  285  0  0 100  0  0 

未使用内存free在逐渐减小，而 buffer 和 cache 基本不变，系统中使用的内存一直在升高。但这并不能说明有内存泄漏，因为应用程序运行中需要的内存也可能会增大。比如说，程序中如果用了一个动态增长的数组来缓存计算结果，占用内存自然会增长

# -a 表示显示每个内存分配请求的大小以及地址，-p 指定案例应用的PID号
$ docker cp app:/app /app
$ /usr/share/bcc/tools/memleak -p $(pidof app) -a
Attaching to pid 12512, Ctrl+C to quit.
[03:00:41] Top 10 stacks with outstanding allocations:
    addr = 7f8f70863220 size = 8192
    addr = 7f8f70861210 size = 8192
    addr = 7f8f7085b1e0 size = 8192
    addr = 7f8f7085f200 size = 8192
    addr = 7f8f7085d1f0 size = 8192
    40960 bytes in 5 allocations from stack
        fibonacci+0x1f [app]
        child+0x4f [app]
        start_thread+0xdb [libpthread-2.27.so] 
        
fibonacci() 函数分配的内存没释放，修复之后再次运行
 
# 清理原来的案例应用
$ docker rm -f app
# 运行修复后的应用
$ docker run --name=app -itd feisky/app:mem-leak-fix
# 重新执行 memleak工具检查内存泄漏情况
$ /usr/share/bcc/tools/memleak -a -p $(pidof app)
Attaching to pid 18808, Ctrl+C to quit.
[10:23:18] Top 10 stacks with outstanding allocations:
[10:23:23] Top 10 stacks with outstanding allocations: 
```

### 为什么系统的Swap变高了

内存资源紧张时会导致两种结果：

* OOM（Out Of Memory）：系统杀死占用大量内存的进程，释放这些内存，再分配给其他更需要的进程
* 内存回收：系统释放掉可以回收的内存，如缓存和缓冲区，就属于可回收内存。它们在内存管理中，通常被叫做文件页（File-backed Page）。大部分文件页，都可以直接回收，以后有需要时，再从磁盘重新读取就可以了。而那些被应用程序修改过，并且暂时还没写入磁盘的数据（也就是脏页），就得先写入磁盘，然后才能进行内存释放。这些脏页，一般可以通过两种方式写入磁盘
  * 在应用程序中，通过系统调用 fsync  ，把脏页同步到磁盘中
  * 由内核线程 pdflush 负责这些脏页的刷新

文件页：除了缓存和缓冲区，通过内存映射获取的文件映射页，也是一种常见的文件页。它也可以被释放掉，下次再访问的时候，从文件重新读取

匿名页（Anonymous Page）：如果这些内存在分配后很少被访问，Swap 把这些不常访问的内存先写到磁盘中，然后释放这些内存，给其他更需要的进程使用。再次访问这些内存时，重新从磁盘读入内存就可以了

##### Swap 原理

* 换出：就是把进程暂时不用的内存数据存储到磁盘中，并释放这些数据占用的内存
* 换入：则是在进程再次访问这些内存的时候，把它们从磁盘读到内存中来

笔记本电脑的休眠和快速开机的功能，也基于 Swap 。休眠时，把系统的内存存入磁盘，这样等到再次开机时，只要从磁盘中加载内存就可以。这样就省去了很多应用程序的初始化过程，加快了开机速度

* 直接内存回收：有新的大块内存分配请求，但是剩余内存不足。这个时候系统就需要回收一部分内存（比如前面提到的缓存），进而尽可能地满足新内存请求
* 定期回收内存：也就是 kswapd0。为了衡量内存的使用情况，kswapd0 定义了三个内存阈值（watermark，也称为水位），分别是页最小阈值（pages_min）、页低阈值（pages_low）和页高阈值（pages_high）。剩余内存，则使用 pages_free 表示

![img](https://static001.geekbang.org/resource/image/c1/20/c1054f1e71037795c6f290e670b29120.png)

* pages_free < pages_min：说明进程可用内存都耗尽了，只有内核才可以分配内存

* pages_min < pages_free < pages_low：说明内存压力比较大，这时 kswapd0 会执行内存回收，直到剩余内存大于高阈值为止

* pages_low < pages_free < pages_high：说明内存有一定压力，但还可以满足新内存请求
* pages_free > pages_high：说明剩余内存比较多，没有内存压力

页最小阈值，通过内核选项 /proc/sys/vm/min_free_kbytes 来间接设置。而其他两个阈值，都是根据页最小阈值计算生成的，计算方法如下 ：

```
pages_low = pages_min*5/4
pages_high = pages_min*3/2
```

##### NUMA 与 Swap

Swap 升高，可是在分析系统的内存使用时，却很可能发现，系统剩余内存还多着呢。为什么剩余内存很多的情况下，也会发生 Swap ？这正是处理器的 NUMA （Non-Uniform Memory Access）架构导致的。在 NUMA 架构下，多个处理器被划分到不同 Node 上，且每个 Node 都拥有自己的本地内存空间。而同一个 Node 内部的内存空间，又可以进一步分为不同的内存域（Zone），比如直接内存访问区（DMA）、普通内存区（NORMAL）、伪内存区（MOVABLE）等，如下图所示

![img](https://static001.geekbang.org/resource/image/be/d9/be6cabdecc2ec98893f67ebd5b9aead9.png)

分析工具：

* numactl：查看处理器在 Node 的分布情况

```
numactl --hardware
available: 1 nodes (0)
node 0 cpus: 0 1 2 3
node 0 size: 7977 MB
node 0 free: 4416 MB
...

只有一个 Node 0 ，而且编号为 0 1 2 3 的四个 CPU， 都位于 Node 0 上，以及内存大小和剩余内存大小
```

三个内存阈值（页最小阈值、页低阈值和页高阈值），都可以通过内存域在 proc 文件系统中的接口 /proc/zoneinfo 来查看

```
cat /proc/zoneinfo
...
Node 0, zone   Normal
 pages free     227894
       min      14896
       low      18620
       high     22344
...
     nr_free_pages 227894
     nr_zone_inactive_anon 11082
     nr_zone_active_anon 14024
     nr_zone_inactive_file 539024
     nr_zone_active_file 923986
...

pages 处的 min、low、high，就是三个内存阈值，而 free 是剩余内存页数，它跟后面的 nr_free_pages 相同nr_zone_active_anon 和 nr_zone_inactive_anon，分别是活跃和非活跃的匿名页数
nr_zone_active_file 和 nr_zone_inactive_file，分别是活跃和非活跃的文件页数

剩余内存远大于页高阈值，所以此时的 kswapd0 不会回收内存
```

某个 Node 内存不足时，系统可以从其他 Node 寻找空闲内存，也可以从本地内存中回收内存。具体选哪种模式，可以通过 /proc/sys/vm/zone_reclaim_mode 来调整：

* 默认的 0 ，也就是刚刚提到的模式，表示既可以从其他 Node 寻找空闲内存，也可以从本地回收内存
* 1、2、4 都表示只回收本地内存，2 表示可以回写脏数据回收内存，4 表示可以用 Swap 方式回收内存

##### swappiness

回收的内存既包括了文件页，又包括了匿名页

* 对文件页的回收，当然就是直接回收缓存，或者把脏页写回磁盘后再回收
* 对匿名页的回收，其实就是通过 Swap 机制，把它们写入磁盘后再释放内存

既然有两种不同的内存回收机制，在实际回收内存时Linux 提供了一个  /proc/sys/vm/swappiness 选项，用来调整使用 Swap 的积极程度：swappiness 的范围是 0-100，数值越大，越积极使用 Swap，也就是更倾向于回收匿名页；数值越小，越消极使用 Swap，也就是更倾向于回收文件页。虽然 swappiness 的范围是 0-100，这并不是内存的百分比，而是调整 Swap 积极程度的权重，即使把它设置成 0，当剩余内存 + 文件页小于页高阈值时，还是会发生 Swap

