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

