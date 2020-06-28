### 搭建操作系统实验环境

1. 创建虚拟机：通过OpenStack的 libvirt创建和管理qemu虚拟机

```
apt-get install libvirt-bin
apt-get install virtinst
...
```

2. 下载源代码

```
apt-get install linux-source-4.15.0
...
```

3. 编译内核

```
apt-get install libncurses5-dev libssl-dev bison flex libelf-dev gcc make openssl libc6-dev

make menuconfig

nohup make -j8 > make1.log 2>&1 &
nohup make modules_install > make2.log 2>&1 &
nohup make install > make3.log 2>&1 &
```

自定义一个系统调用：

1. `arch/x86/entry/syscalls/syscall_64.tbl`

```
332     common  statx                   sys_statx
333     64      sayhelloworld           sys_sayhelloworld
```

2. `include/linux/syscalls.h`

```
asmlinkage long sys_statx(int dfd, const char __user *path, unsigned flags,
                          unsigned mask, struct statx __user *buffer);

asmlinkage int sys_sayhelloworld(char * words, int count);
```

3. 系统调用的实现，方便起见，不再用 SYSCALL_DEFINEx 系列的宏来定义，直接在 kernel/sys.c 中实现

```
asmlinkage int sys_sayhelloworld(char * words, int count){
  int ret;
  char buffer[512];
  if(count >= 512){
    return -1;
  }
  copy_from_user(buffer, words, count);
  ret=printk("User Mode says %s to the Kernel Mode!", buffer);
  return ret;
}
```

##### GDB调试

常用命令参数：

```
l，即 list，用于显示多行源代码
b，即 break，用于设置断点
r，即 run，用于开始运行程序
n，即 next，用于执行下一条语句。如果该语句为函数调用，则不会进入函数内部执行
p，即 print，用于打印内部变量值
s，即 step，用于执行下一条语句。如果该语句为函数调用，则进入函数，执行其中的第一条语句
c，即 continue，用于继续程序的运行，直到遇到下一个断点
bt，即 backtrace，用于查看函数调用信息
q，即 quit，用于退出 gdb 环境
```

 通过宿主机上的 gdb 来 debug 虚拟机里面的内核的步骤：

1. 需要将 debug 所需信息也放入二进制文件里面去。把“CONFIG_DEBUG_INFO”和“CONFIG_FRAME_POINTER”两个变量设置为 yes
2. 安装 gdb。kernel 运行在 qemu 虚拟机里面，gdb 运行在宿主机上，所以应该在宿主机上进行安装
3. 找到 gdb 要运行的那个内核的二进制文件。根据 grub 里面的配置，应该在 /boot/vmlinuz-4.15.18 
4. 修改 qemu 的启动参数和里面虚拟机的启动参数，从而使得 gdb 可以远程 attach 到 qemu 里面的内核上
5. 使用 gdb 运行内核的二进制文件，执行 gdb vmlinux

##### kernel的启动流程

1. 在 1M 空间最上面的 0xF0000 到 0xFFFFF 这 64K 映射给 ROM，通过读这部分地址，可以访问 BIOS 
2. 在启动盘的第一个扇区，512K 的大小，为 MBR（Master Boot Record，主引导记录 / 扇区）。保存了 boot.img，BIOS 会将他加载到内存中的 0x7c00 来运行
3. boot.img 会加载 grub2 的另一个镜像 core.img
4. core.img 由 lzma_decompress.img、diskboot.img、kernel.img 和一系列的模块组成，功能比较丰富
5. boot.img 将控制权交给 diskboot.img 后，diskboot.img 的任务就是将 core.img 的其他部分加载进来，先是解压缩程序 lzma_decompress.img，再往下是 kernel.img，最后是各个模块 module 对应的映像
6. kernel.img 里面的 grub_main 会展示操作系统的列表，可以进行选择
7. kernel_init 运行 1 号进程。 它会在用户态运行
8. kthreadd 运行 2 号进程。它会在内核态运行

##### 进程调度

1. fork 一个子项目，然后调用 exec 系统调用，到了内核里面，通过 load_elf_binary 将ELF 文件（ELF Header 的部分，包含指令的代码段的部分，包含全局变量的数据段的部分）加载到子进程内存中，交给一个 CPU 执行
2. TASK_RUNNING 表示进程在时刻准备运行的状态。睡眠状态有两种。一种是 TASK_INTERRUPTIBLE，可中断的睡眠状态。浅睡眠虽然在睡眠，等条件成熟，进程可以被唤醒。另一种是 TASK_UNINTERRUPTIBLE，不可中断的睡眠状态。深度睡眠不可被唤醒，只能死等条件满足。还有一种新的进程睡眠状态，TASK_KILLABLE，可以终止的新睡眠状态。进程处于这种状态中，运行原理类似 TASK_UNINTERRUPTIBLE，只不过可以响应致命信号，即虽然在深度睡眠，但是可以被干掉
3. 一旦一个进程要结束，先进入 EXIT_ZOMBIE 状态，此时其父进程还没有使用 wait() 等系统调用来获知他的终止信息，它就成了僵尸进程。EXIT_DEAD 是进程的最终状态
4. 在 Linux 里面，无论是进程还是线程，到了内核里面，统一叫任务，由统一的结构 task_struct 进行管理
5. 一个 CPU 上有一个队列，队列里面是一系列 sched_entity，每个 sched_entity 都属于一个 task_struct，代表进程或者线程
6. 每一个 CPU 小伙伴每过一段时间，都要判断该执行哪个任务，CFS 全称是 Completely Fair Scheduling，完全公平调度，由fair_sched_class执行
7. CPU 会提供一个时钟，过一段时间就触发一个时钟中断 Tick。CFS 会为每一个进程安排一个虚拟运行时间 vruntime，函数 update_curr 用于更新进程运行的统计量 vruntime ，CFS 还需要一个数据结构来对 vruntime 进行排序，使用红黑树找出最小的那个。红黑树的节点是 sched_entity，里面包含 vruntime
8. 调度算法的本质就是解决下一个进程应该谁运行的问题，逻辑在 fair_sched_class.pick_next_task 中完成
9. 切换任务的场景：
   * 主动：sleep或者等待某个 I/O 事件。要主动让出 CPU，主动调用 schedule() 函数。通过 fair_sched_class.pick_next_task，在红黑树形成的队列上取出下一个进程，然后调用 context_switch 进行进程上下文切换，进程上下文切换主要做两件事情，一是切换进程空间，即进程的内存，二是切换寄存器和 CPU 上下文
   * 被动：执行时间太长。时钟 Tick 的时候，时钟中断处理函数会调用 scheduler_tick()，调用 fair_sched_class 的 task_tick_fair，在这里面会调用 update_curr 更新运行时间。当发现当前进程应该被抢占，不能直接把他踢下来，而是把他标记为应该被抢占，打上一个标签 TIF_NEED_RESCHED。另外一个可能抢占的场景，当一个进程被唤醒的时候。一个进程在等待一个 I/O 的时候，会主动放弃 CPU。但是，当 I/O 到来的时候，进程往往会被唤醒。当被唤醒的进程优先级高于 CPU 上的当前进程，就会触发抢占。如果应该发生抢占，而也是将当前进程标记为应该被抢占，打上一个标签 TIF_NEED_RESCHED

10. 真正的抢占还是需要上下文切换，需要一个时刻，让正在运行中的进程有机会调用一下 schedule。有以下四个时机：
    * 对于用户态的进程来讲，从系统调用中返回的那个时刻，是一个被抢占的时机
    * 对于用户态的进程来讲，从中断中返回的那个时刻，也是一个被抢占的时机
    * 对内核态的执行中，被抢占的时机一般发生在 preempt_enable() 中。在内核态的执行中，有的操作是不能被中断的，所以在进行这些操作之前，总是先调用 preempt_disable() 关闭抢占。再次打开的时候，就是一次内核态代码被抢占的机会
    * 在内核态也会遇到中断的情况，当中断返回的时候，返回的仍然是内核态。这个时候也是一个执行抢占的时机

##### 内存管理

![img](https://static001.geekbang.org/resource/image/8f/49/8f158f58dda94ec04b26200073e15449.jpeg)

![img](https://static001.geekbang.org/resource/image/3f/4f/3fa8123990e5ae2c86859f70a8351f4f.jpeg)

![img](https://static001.geekbang.org/resource/image/4e/9d/4ed91c744220d8b4298237d2ab2eda9d.jpeg)

![img](https://static001.geekbang.org/resource/image/ab/40/abbcafe962d93fac976aa26b7fcb7440.jpg)

![img](https://static001.geekbang.org/resource/image/83/c3/83a5de160088a2e23e7c1a76c013efc3.jpg)

![img](https://static001.geekbang.org/resource/image/b6/b8/b6960eb0a7eea008d33f8e0c4facc8b8.jpg)

![img](https://static001.geekbang.org/resource/image/42/0b/42eff3e7574ac8ce2501210e25cd2c0b.jpg)

1. 物理地址对于进程不可见，操作系统会给进程分配一个虚拟地址。所有进程看到的这个地址都是一样的，里面的内存都是从 0 开始编号。当程序要访问虚拟地址的时候，由内核的数据结构进行转换，转换成不同的物理地址，不同的进程运行的时候，写入的是不同的物理地址
2. 内存管理包含三个部分：物理内存的管理、虚拟地址的管理、虚拟地址和物理地址如何映射
3. 现代计算机很少是对称多处理器SMP结构，大多是非统一内存访问NUMA结构。分为若干节点，每个节点用 struct pglist_data 表示。每个节点里面再分区域。ZONE_NORMAL 是最常用的区域，ZONE_MOVABLE 是可移动区域等等，以此来避免内存碎片
4. 每个区域里面再分页，默认大小为 4KB。空闲页放在 struct free_area，每一页用 struct page 表示。所有空闲页分组为 11 个页块链表，每个块链表分别包含很多个大小的页块，有 1、2、4、8、16、32、64、128、256、512 和 1024 个连续页的页块。最大可以申请 1024 个连续页，对应 4MB 大小的连续内存。每个页块的第一个页的物理地址是该页块大小的整数倍
5. 把物理页面分成一块一块大小相同的页的一个好处是，当有的内存页面长时间不用了，可以暂时写到硬盘上，称为换出。需要的时候，再加载进来，叫作换入。这样可以扩大可用物理内存的大小，提高物理内存的利用率。在内核里面，有一个进程 kswapd，可以根据物理页面的使用情况，对页面进行换入换出
6. 这仅仅是物理地址的管理，对于虚拟地址，如果是 32 位，有 2^32 = 4G 的内存空间，不管内存是不是真的有 4G。如果是 64 位，在 x86_64 下面，其实只使用了 48 位。地址长度对应了 256TB 的地址空间
7. 这么大的虚拟空间，一部分用来放内核的东西，称为内核空间；一部分用来放进程的东西，称为用户空间。用户空间在下，在低地址；内核空间在上，在高地址
8. 从最低位开始排起，先是 Text Segment、Data Segment 和 BSS Segment。Text Segment 是存放二进制可执行代码的位置，Data Segment 存放静态常量，BSS Segment 存放未初始化的静态变量。接下来是堆段，堆往高地址增长，是用来动态分配内存的区域，malloc 就是在这里面分配的。接下来的区域是 Memory Mapping Segment。可以用来把文件映射进内存用的，如果二进制的执行文件依赖于某个动态链接库，在这个区域里面将 so 文件映射到了内存中。再下面就是栈地址段了，主线程的函数调用的函数栈就是用这里
9. 如果需要进行更高权限的工作，就需要调用系统调用进入内核。无论是从哪个进程进来的，看到的是同一个内核空间，看到的是同一个进程列表
10. 内核的代码访问内核的数据结构，大部分的情况下都是使用虚拟地址的。虽然内核代码权限很大，但是能够使用的虚拟地址范围也只能在内核空间，在内核里面也会有内核的代码，同样有 Text Segment、Data Segment 和 BSS Segment，内核代码也是 ELF 格式
11. 还需要实现从虚拟地址到物理地址的转换
12. 虚拟地址分为两部分，页号和页内偏移。页号作为页表的索引，页表包含物理页每页所在物理内存的基地址。这个基地址与页内偏移的组合就形成了物理内存地址
13. 32 位环境下，虚拟地址空间共 4GB。如果分成 4KB 一个页，那就是 1M 个页。每个页表项需要 4 个字节来存储，那么整个 4GB 空间的映射就需要 4MB 的内存来存储映射表。如果每个进程都有自己的映射表，100 个进程就需要 400MB 的内存，太大了
14. 而页表中所有页表项必须提前建好，并且要求是连续的。可以将页表再分页，4G 的空间需要 4M 的页表来存储映射。把 4M 分成 1K（1024）个 4K，每个 4K 又能放在一页里面，这样 1K 个 4K 就是 1K 个页，这 1K 个页也需要一个表进行管理，称为页目录表，页目录表里面有 1K 项，每项 4 个字节，页目录表大小也是 4K
15. 页目录有 1K 项，用 10 位就可以表示访问页目录的哪一项，即 4K 的页表项。每个页表项也是 4 个字节，因而一整页的页表项是 1k 个。再用 10 位就可以表示访问页表项的哪一项，页表项中的一项对应的就是一个页，是存放数据的页，这个页的大小是 4K，用 12 位可以定位这个页内的任何一个位置。加起来正好 32 位，就是用前 10 位定位到页目录表中的一项。将这一项对应的页表取出来共 1k 项，再用中间 10 位定位到页表中的一项，将这一项对应的存放数据的页取出来，再用最后 12 位定位到页中的具体位置访问数据
16. 假设只给一个进程分配了一个数据页。如果只使用页表，也需要完整的 1M 个页表项共 4M 的内存，但是如果使用了页目录，页目录需要 1K 个全部分配，占用内存 4K，但是里面只有一项使用了。到了页表项，只需要分配能够管理那个数据页的页表项页就可以了，也就是说，最多 4K，这样内存就节省多了
17. 对于 64 位的系统，两级肯定不够，就变成了四级目录，分别是全局页目录项 PGD（Page Global Directory）、上层页目录项 PUD（Page Upper Directory）、中间页目录项 PMD（Page Middle Directory）和页表项 PTE（Page Table Entry）

##### 存储管理

![img](https://static001.geekbang.org/resource/image/3c/73/3c506edf93b15341da3db658e9970773.jpg)

![img](https://static001.geekbang.org/resource/image/80/7f/80e152fe768e3cb4c84be62ad8d6d07f.jpg)

![img](https://static001.geekbang.org/resource/image/aa/c0/aa9d074d9819f0eb513e11014a5772c0.jpg)

![img](https://static001.geekbang.org/resource/image/a3/e5/a364f9a9ac045c5d4c1c5a7dfa9ca6e5.png)

1. 文件系统要有严格的组织形式，使得文件能够以块为单位进行存储、文件系统中也要有索引区和缓存层、文件应该用文件夹的形式组织起来，方便管理和查询、Linux 内核要在自己的内存里面维护一套数据结构，来保存哪些文件被哪些进程打开和使用
2. 每一个进程，打开的文件都有一个文件描述符。files_struct 里面会有文件描述符数组。每个一个文件描述符是这个数组的下标，里面的内容指向一个 struct file 结构，表示打开的文件。这个结构里面有这个文件对应的 inode，最重要的是这个文件对应的操作 file_operation
3. 每一个打开的文件，都有一个 dentry 对应，虽然叫 directory entry，但不仅仅表示文件夹，也表示文件。最重要的作用就是指向这个文件对应的 inode，inode 结构表示硬盘上的 inode，包括块设备号等。它对应的操作保存在 inode operations 里面。真正写入数据，是写入硬盘上的文件系统，例如 ext4 文件系统
4. 如果 file 结构是一个文件打开以后才创建的，dentry 是放在一个 dentry cache 里面的。文件关闭后依然存在，因而可以更长期的维护内存中的文件的表示和硬盘上文件的表示之间的关系
5. 设备控制器屏蔽设备差异，CPU 并不直接和设备打交道，中间有一个叫作设备控制器（Device Control Unit）的组件。例如，硬盘有磁盘控制器、USB 有 USB 控制器、显示器有视频控制器等
6. 驱动程序屏蔽设备控制器差异，设备控制器不属于操作系统的一部分，但是设备驱动程序属于操作系统的一部分。操作系统的内核代码可以像调用本地代码一样调用驱动程序的代码，而驱动程序的代码需要发出特殊的面向设备控制器的指令，才能操作设备控制器
7. 中断控制器统一外部事件处理
8. 文件系统接口屏蔽驱动程序的差异
9. 所有设备都在 /dev/ 文件夹下面，创建一个特殊的设备文件，它也有 inode，但是不关联到硬盘或任何其他存储介质上的数据，而是建立了与某个设备驱动程序的连接。有了文件系统接口之后，不但可以通过文件系统的命令行操作设备，也可以通过程序，调用 read、write 函数，像读写文件一样操作设备
10. 块设备类型非常多，不能在文件系统以下，直接对接各种各样的块设备驱动程序，这样会使得文件系统的复杂度非常高。中间加一层通用块层，将与块设备相关的通用逻辑放在这一层，维护与设备无关的块的大小，然后通用块层下面对接各种各样的驱动程序

##### 进程间通信

1. 管道：两个进程之间建立一条单向的通道，是一段缓存，它会将前一个命令的输出，作为后一个命令的输入，不适合进程间频繁的交换数据
2. 消息队列：发送数据时，会分成一个一个独立的数据单元，也就是消息体，每个消息体都是固定大小的存储块，在字节流上不连续
3. 共享内存：拿出一块虚拟地址空间来，映射到相同的物理内存中。这个进程写入的东西，另外一个进程马上就能看到，需要配合信号量进行同步
4. 信号：处理紧急事务

##### 网络管理

![img](https://static001.geekbang.org/resource/image/92/0e/92f8e85f7b9a9f764c71081b56286e0e.png)

![img](https://static001.geekbang.org/resource/image/d3/5c/d34e667d1c3340deb8c82a2d44f2a65c.png)

![img](https://static001.geekbang.org/resource/image/1a/e5/1a8450f1fcda83b75c9ba301ebf9fbe5.jpg)

1. IP层：网络包从一个起始的 IP 地址，经过多个网络，通过多次路由器转发，到达目标 IP 地址
2. 数据链路层：MAC就是每个网卡都有的唯一的硬件地址（不绝对唯一，相对大概率唯一即可，类比 UUID）。虽然是一个地址，但是没有全局定位功能。MAC 地址的定位功能局限在一个网络里面，也即同一个网络号下的 IP 地址之间，可以通过 MAC 进行定位和通信。从 IP 地址获取 MAC 地址要通过 ARP 协议，是通过在本地发送广播包，获得的 MAC 地址。一旦跨网络通信，虽然 IP 地址保持不变，但是 MAC 地址每经过一个路由器就要换一次
3. 物理层：物理设备
4. 传输层：如TCP、UDP。在 IP 层的代码逻辑中，仅仅负责数据从一个 IP 地址发送给另一个 IP 地址，丢包、乱序、重传、拥塞，IP 层都不管。处理这些问题的代码逻辑写在了传输层的 TCP 协议里面，不同的应用监听不同的端口
5. 应用层：如HTTP、Servlet
6. 数据链路层、网络层、传输层都是在Linux内核里面处理，Socket是应用层和内核互通的机制。Socket哪一层都不属于，它属于操作系统的概念，而非网络协议分层的概念。两者需要通过Socket跨内核态和用户态通信
7. 将请求封装为 HTTP 协议，通过 Socket 发送到内核。内核的网络协议栈里面，在 TCP 层创建用于维护连接、序列号、重传、拥塞控制的数据结构，将 HTTP 包加上 TCP 头，发送给 IP 层，IP 层加上 IP 头，发送给 MAC 层，MAC 层加上 MAC 头，从硬件网卡发出去
8. 网络包会被转发到目标服务器，它发现 MAC 地址匹配，就将 MAC 头取下来，交给上一层。IP 层发现 IP 地址匹配，将 IP 头取下来，交给上一层。TCP 层会根据 TCP 头中的序列号等信息，发现它是一个正确的网络包，就会将网络包缓存起来，等待应用层的读取。应用层通过 Socket 监听某个端口，因而读取的时候，内核会根据 TCP 头中的端口号，将网络包发给相应的应用
9. 虚拟化的方式：完全虚拟化（虚拟机，qemu）、硬件辅助虚拟化（qemu-kvm）、半虚拟化（网络virtio_net，存储virtio_blk）
10. 最重要的四种硬件资源 CPU、内存、存储和网络。对于存储，面对整个数据中心成千上万台机器，需要一个调度器，实现对于物理资源的统一管理，这就是 Kubernetes，数据中心的操作系统
11. 对于 CPU 和内存，可以通过 Docker 技术完成，容器实现封闭的环境主要要靠两种技术， namespace和cgroup

