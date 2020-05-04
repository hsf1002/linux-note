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

