### 输入输出系统

##### 用设备控制器屏蔽设备差异

CPU 并不直接和设备打交道，它们中间有一个叫作设备控制器（Device Control Unit）的组件，例如硬盘有磁盘控制器、USB 有 USB 控制器、显示器有视频控制器等。控制器有点儿像一台小电脑。它有它的芯片，类似小 CPU，执行自己的逻辑。也有它的寄存器。这样 CPU 就可以通过写这些寄存器，对控制器下发指令，通过读这些寄存器，查看控制器对于设备的操作状态。

输入输出设备我们大致可以分为两类：

* 块设备：将信息存储在固定大小的块中，每个块都有自己的地址。硬盘就是常见的块设备，由于块设备传输的数据量比较大，控制器里往往会有缓冲区。CPU 写入缓冲区的数据攒够一部分，才会发给设备。CPU 读取的数据，也需要在缓冲区攒够一部分，才拷贝到内存
* 字符设备：发送或接收的是字节流。而不用考虑任何块结构，没有办法寻址。鼠标就是常见的字符设备

控制器的寄存器一般会有状态标志位，检测状态标志位的方式：

* 轮询等待，就是一直查，一直查，直到完成

* 中断

  * 软中断：例如代码调用 INT 指令触发
  * 硬件中断：硬件通过中断控制器触发

  ![img](https://static001.geekbang.org/resource/image/5d/55/5d9290f08847685d65bc3edd88242855.jpg)

有的设备需要读取或者写入大量数据。如果所有过程都让 CPU 协调的话，就需要占用 CPU 大量的时间，如磁盘。这种类型的设备需要支持 DMA 功能，允许设备在 CPU 不参与的情况下，能够自行完成对内存的读写。实现 DMA 机制需要有个 DMA 控制器帮 CPU 来做协调。CPU 只需要对 DMA 控制器下指令，说它想读取多少数据，放在内存的某个地方就可以了，接下来 DMA 控制器会发指令给磁盘控制器，读取磁盘上的数据到指定的内存位置，传输完毕之后，DMA 控制器发中断通知 CPU 指令完成，CPU 就可以直接用内存里面现成的数据了

![img](https://static001.geekbang.org/resource/image/1e/35/1ef05750bc9ff87a3330104802965335.jpeg)

##### 用驱动程序屏蔽设备控制器差异

设备控制器不属于操作系统的一部分，但是设备驱动程序属于操作系统的一部分。操作系统的内核代码可以像调用本地代码一样调用驱动程序的代码，而驱动程序的代码需要发出特殊的面向设备控制器的指令，才能操作设备控制器

![img](https://static001.geekbang.org/resource/image/7b/68/7bf96d3c8e3a82cdac9c7629b81fa368.png)

一般的流程是，一个设备驱动程序初始化的时候，要先注册一个该设备的中断处理函数。中断返回的那一刻是进程切换的时机。中断的时候，触发的函数是 do_IRQ。这个函数是中断处理的统一入口。在这个函数里面，可以找到设备驱动程序注册的中断处理函数 Handler，然后执行它进行中断处理

![img](https://static001.geekbang.org/resource/image/aa/c0/aa9d074d9819f0eb513e11014a5772c0.jpg)

对于块设备来讲，在驱动程序之上，文件系统之下，还需要一层通用设备层，将与块设备相关的通用逻辑放在这一层，维护与设备无关的块的大小，然后通用块层下面对接各种各样的驱动程序

![img](https://static001.geekbang.org/resource/image/3c/73/3c506edf93b15341da3db658e9970773.jpg)

##### 用文件系统接口屏蔽驱动程序的差异

从硬件设备到设备控制器，到驱动程序，到通用块层，到文件系统，层层屏蔽不同的设备的差别，最终到这里涉及对用户使用接口，也要统一。虽然操作设备都是基于文件系统的接口，也要有一个统一的标准

* 首先要统一的是设备名称。所有设备都在 /dev/ 文件夹下面创建一个特殊的设备文件。这个设备特殊文件也有 inode，但是它不关联到硬盘或任何其他存储介质上的数据，而是建立了与某个设备驱动程序的连接。假设是 /dev/sdb，这是一个设备文件。这个文件本身和硬盘上的文件系统没有任何关系。这个设备本身也不对应硬盘上的任何一个文件，/dev/sdb 其实是在一个特殊的文件系统 devtmpfs 中。但是当 /dev/sdb 格式化成一个文件系统 ext4 的时候，就会将它 mount 到一个路径下面。例如在 /mnt/sdb 下面。这个时候 /dev/sdb 还是一个设备文件在特殊文件系统 devtmpfs 中，而 /mnt/sdb 下面的文件才是在 ext4 文件系统中，只不过这个设备是在 /dev/sdb 设备上的。如果用文件的操作作用于 /dev/sdb 的话，会无法操作文件系统上的文件，操作的是这个设备本身

* 首先是第一位字符。如果是字符设备文件，则以 c 开头，如果是块设备文件，则以 b 开头。其次是两个号，一个是主设备号，一个是次设备号。主设备号定位设备驱动程序，次设备号作为参数传给启动程序。mem、null、random、urandom、zero 都是用同样的主设备号 1，也就是它们使用同样的字符设备驱动，而 vda、vda1、vdb、vdc 也是同样的主设备号，它们使用同样的块设备驱动

```
# ls -l /dev
crw------- 1 root root      5,   1 Dec 14 19:53 console
crw-r----- 1 root kmem      1,   1 Dec 14 19:53 mem
crw-rw-rw- 1 root root      1,   3 Dec 14 19:53 null
crw-r----- 1 root kmem      1,   4 Dec 14 19:53 port
crw-rw-rw- 1 root root      1,   8 Dec 14 19:53 random
crw--w---- 1 root tty       4,   0 Dec 14 19:53 tty0
crw--w---- 1 root tty       4,   1 Dec 14 19:53 tty1
crw-rw-rw- 1 root root      1,   9 Dec 14 19:53 urandom
brw-rw---- 1 root disk    253,   0 Dec 31 19:18 vda
brw-rw---- 1 root disk    253,   1 Dec 31 19:19 vda1
brw-rw---- 1 root disk    253,  16 Dec 14 19:53 vdb
brw-rw---- 1 root disk    253,  32 Jan  2 11:24 vdc
crw-rw-rw- 1 root root      1,   5 Dec 14 19:53 zero
```

* 有了设备文件，可以使用对于文件的操作命令和 API 来操作文件了。例如，使用 cat 命令，可以读取 /dev/random 和 /dev/urandom 的数据流，可以用 od 命令转换为十六进制后查看

```
cat /dev/urandom | od -x
```

* 用命令 lsmod，查看有没有加载过相应的内核模块，用 insmod 安装内核模块。内核模块的后缀一般是 ko

```
insmod openvswitch.ko
```

一旦有了驱动，我们就可以通过命令 mknod 在 /dev 文件夹下面创建设备文件

```
# filename 就是 /dev 下面的设备名称，type 就是 c 为字符设备，b 为块设备，major 就是主设备号，minor 就是次设备号。新创建的设备文件就和上面加载过的驱动关联起来，就可以通过操作设备文件来操作驱动程序，从而操作设备
mknod filename type major minor
```

* 用上述命令创建新的设备太麻烦了，简单的方式要用到另一个管理设备的文件系统，也就是 /sys 路径下面的 sysfs 文件系统。它把实际连接到系统上的设备和总线组成了一个分层的文件系统。这个文件系统是当前系统上实际的设备数的真实反映
  * /sys/devices 是内核对系统中所有设备的分层次的表示
  * /sys/dev 目录下一个 char 文件夹，一个 block 文件夹，分别维护一个按字符设备和块设备的主次号码 (major:minor) 链接到真实的设备 (/sys/devices 下) 的符号链接文件
  * /sys/block 是系统中当前所有的块设备
  * /sys/module 有系统中所有模块的信息

有了 sysfs 以后，还需要一个守护进程 udev。当一个设备新插入系统的时候，内核会检测到这个设备，并会创建一个内核对象 kobject 。 这个对象通过 sysfs 文件系统展现到用户层，同时内核还向用户空间发送一个热插拔消息。udevd 会监听这些消息，在 /dev 中创建对应的文件

![img](https://static001.geekbang.org/resource/image/62/90/6234738aac8d5897449e1a541d557090.jpg)

有了文件系统接口之后，不但可以通过文件系统的命令行操作设备，也可以通过程序，调用 read、write 函数，像读写文件一样操作设备。但是有些任务只使用读写很难完成，例如检查特定于设备的功能和属性，超出了通用文件系统的限制。对于设备来讲，还有一种接口称为 ioctl，表示输入输出控制接口，是用于配置和修改特定设备属性的通用接口

![img](https://static001.geekbang.org/resource/image/80/7f/80e152fe768e3cb4c84be62ad8d6d07f.jpg)

### 字符设备

输入字符设备，鼠标，代码 drivers/input/mouse/logibm

输出字符设备，打印机，代码 drivers/char/lp.c 

##### 内核模块

设备驱动程序是一个内核模块，以 ko 的文件形式存在，可以通过 insmod 加载到内核中，主要步骤：

1. 头文件部分。一般的内核模块，都需要 include 下面两个头文件：

```
#include <linux/module.h>
#include <linux/init.h>
```

2. 定义一些用于处理内核模块的主要逻辑。例如打开、关闭、读取、写入设备的函数或者响应中断的函数
3. 定义一个 file_operations 结构，lp.c 里面就定义了这样一个结构

```
static const struct file_operations lp_fops = {
  .owner    = THIS_MODULE,
  .write    = lp_write,
  .unlocked_ioctl  = lp_ioctl,
#ifdef CONFIG_COMPAT
  .compat_ioctl  = lp_compat_ioctl,
#endif
  .open    = lp_open,
  .release  = lp_release,
#ifdef CONFIG_PARPORT_1284
  .read    = lp_read,
#endif
  .llseek    = noop_llseek,
};
```

在 logibm.c 里面，找不到这样的结构，是因为它属于众多输入设备的一种，而输入设备的操作被统一定义在 drivers/input/input.c 里面，logibm.c 只是定义了一些自己独有的操作

```
static const struct file_operations input_devices_fileops = {
  .owner    = THIS_MODULE,
  .open    = input_proc_devices_open,
  .poll    = input_proc_devices_poll,
  .read    = seq_read,
  .llseek    = seq_lseek,
  .release  = seq_release,
};
```

4. 定义整个模块的初始化函数和退出函数，用于加载和卸载这个 ko 的时候调用
5. 调用 module_init 和 module_exit，分别指向上面两个初始化函数和退出函数
6. 声明一下 lisense，调用 MODULE_LICENSE

##### 打开字符设备

![img](https://static001.geekbang.org/resource/image/2e/e6/2e29767e84b299324ea7fc524a3dcee6.jpeg)

字符设备可不是一个普通的内核模块，它有自己独特的行为，要使用一个字符设备：

1. 首先要把写好的内核模块，通过 insmod 加载进内核。此时先调用的是 module_init 调用的初始化函数，注册这个字符设备。__register_chrdev_region，注册字符设备的主次设备号和名称，然后分配一个 struct cdev 结构，将 cdev 的 ops 成员变量指向这个模块声明的 file_operations。然后，cdev_add 会将这个字符设备添加到内核中一个叫作 struct kobj_map *cdev_map 的结构，来统一管理所有字符设备
2. 接下来要通过 mknod 在 /dev 下面创建一个设备文件，有了这个设备文件，才能通过文件系统的接口，对这个设备文件进行操作，mknod 是一个系统调用，会在文件系统上，顺着路径找到 /dev/xxx 所在的文件夹，为这个新创建的设备文件创建一个 dentry。这是维护文件和 inode 之间的关联关系的结构。/dev 下面的文件系统的名称为 devtmpfs，在挂载的时候有两种模式， ramfs和shmem，都是基于内存的文件系统。最终调用init_special_inode
3. 每一个打开的文件都有一个 struct file 结构，会指向一个 dentry 项。dentry 可以用来关联 inode。这个 dentry 就是mknod 的时候创建的。在进程里面调用 open 函数，最终会调用到这个特殊的 inode 的 open 函数，也就是 chrdev_open，这个 inode 的 i_cdev，是否已经关联到 cdev。如果第一次打开，当然没有。inode 里面有 i_rdev，即 dev_t。可以通过它在 cdev_map 中找 cdev。注册过了，所以肯定能够找到。找到后就将 inode 的 i_cdev，关联到找到的 cdev new，找到 cdev 里面有 file_operations，里面有 open 函数，真正打开设备

##### 写入字符设备

像打开一个文件一样打开一个字符设备之后，接下来就是对这个设备的读写。读写的过程是类似的，此处解析打印机的写入过程

![img](https://static001.geekbang.org/resource/image/9b/e2/9bd3cd8a8705dbf69f889ba3b2b5c2e2.jpeg)

1. 写入一个字符设备，就是用文件系统的标准接口 write，参数文件描述符 fd，在内核里面调用的 sys_write，在 sys_write 里面根据文件描述符 fd 得到 struct file 结构。接下来再调用 vfs_write，最终调用lp_write
2. 先是调用 copy_from_user 将数据从用户态拷贝到内核态的缓存中，然后调用 parport_write 写入外部设备。还有一个 schedule 函数，也即写入的过程中，给其他线程抢占 CPU 的机会
3. 如果 count 还是大于 0，也就是数据还没有写完，接着 copy_from_user 和 parport_write，直到写完为止

##### 使用 IOCTL 控制设备

ioctl 是一个系统调用

![img](https://static001.geekbang.org/resource/image/c3/1d/c3498dad4f15712529354e0fa123c31d.jpeg)

fd 是这个设备的文件描述符，cmd 是传给这个设备的命令，arg 是命令的参数。

 cmd 看起来是一个 int，其实他的组成比较复杂：

* 最低八位为 NR，是命令号
* 然后八位是 TYPE，是类型
* 然后十四位是参数的大小
* 最高两位是 DIR，是方向，表示写入、读出，还是读写

ioctl 中会调用 do_vfs_ioctl，对于已经定义好的 cmd，进行相应的处理。如果不是默认定义好的 cmd，则执行默认操作。对于普通文件，调用 file_ioctl；对于其他文件调用 vfs_ioctl，设备驱动程序，所以调用的是 vfs_ioctl，这里调用的是设备驱动的 unlocked_ioctl。对于打印机程序来讲，调用的是 lp_ioctl

一个字符设备要能够工作，需要三部分配合：

1. 第一，有一个设备驱动程序的 ko 模块，里面有模块初始化函数、中断处理函数、设备操作函数。这里面封装了对于外部设备的操作。加载设备驱动程序模块的时候，模块初始化函数会被调用。在内核维护所有字符设备驱动的数据结构 cdev_map 里面注册，就可以很容易根据设备号，找到相应的设备驱动程序
2. 第二，在 /dev 目录下有一个文件表示这个设备，这个文件在特殊的 devtmpfs 文件系统上，也有相应的 dentry 和 inode。这里的 inode 是一个特殊的 inode，里面有设备号。通过它，可以在 cdev_map 中找到设备驱动程序，里面还有针对字符设备文件的默认操作 def_chr_fops
3. 第三，打开一个字符设备文件和打开一个普通的文件有类似的数据结构，有文件描述符、有 struct file、指向字符设备文件的 dentry 和 inode。字符设备文件的相关操作 file_operations 一开始指向 def_chr_fops，在调用 def_chr_fops 里面的 chrdev_open 函数的时候，修改为指向设备操作函数，从而读写一个字符设备文件就会直接变成读写外部设备了

![img](https://static001.geekbang.org/resource/image/fb/cd/fba61fe95e0d2746235b1070eb4c18cd.jpeg)

##### 驱动中断程序

鼠标就是通过中断，将自己的位置和按键信息，传递给设备驱动程序，logibm_interrupt

```
irqreturn_t (*irq_handler_t)(int irq, void * dev_id);
```

* irq 是一个整数，是中断信号

* dev_id 是一个 void * 的通用指针，主要用于区分同一个中断处理函数对于不同设备的处理
* 返回值有三种：IRQ_NONE 表示不是我的中断；IRQ_HANDLED 表示处理完了的中断；IRQ_WAKE_THREAD 表示有一个进程正在等待这个中断，中断处理完了，应该唤醒它

1. 注册中断处理函数：

```
request_irq(logibm_irq, logibm_interrupt, 0, "logibm", NULL)
```

2. 每一个中断，都有一个对中断的描述结构 struct irq_desc。它有一个重要的成员变量是 struct irqaction，用于表示处理这个中断的动作。它里面有 next 指针，这是一个链表，对于这个中断的所有处理动作，都串在这个链表上。一般情况下，所有的 struct irq_desc 都放在一个数组里面，直接按下标查找就可以。如果配置了 CONFIG_SPARSE_IRQ，中断号是不连续的，就不适合用数组保存了，可以放在一棵基数树上。这种结构对于从某个整型 key 找到 value 速度很快，中断信号 irq 是这个整数。很快就能定位到对应的 struct irq_desc。为什么中断信号会有稀疏，也就是不连续的情况呢？这里的 irq 并不是真正的、物理的中断信号，而是一个抽象的、虚拟的中断信号。因为物理的中断信号和硬件关联比较大，中断控制器也是各种各样的。因而就需要有一层中断抽象层。这里虚拟中断信号到中断描述结构的映射，就是抽象中断层的主要逻辑。如果只有一个 CPU，一个中断控制器，则基本能够保证从物理中断信号到虚拟中断信号的映射是线性的，这样用数组表示就没啥问题，但是如果有多个 CPU，多个中断控制器，每个中断控制器各有各的物理中断信号，就没办法保证虚拟中断信号是连续的，所以就要用到基数树了

3. request_irq就是根据中断信号 irq，找到基数树上对应的 irq_desc，然后将新的 irqaction 挂在链表上
4. 真正中断的发生还是要从硬件开始，这里面有四个层次：
   * 第一个层次是外部设备给中断控制器发送物理中断信号
   * 第二个层次是中断控制器将物理中断信号转换成为中断向量 interrupt vector，发给各个 CPU
   * 第三个层次是每个 CPU 都会有一个中断向量表，根据 interrupt vector 调用一个 IRQ 处理函数。注意这里的 IRQ 处理函数还不是上面指定的 irq_handler_t，到这一层还是 CPU 硬件的要求
   * 第四个层次是在 IRQ 处理函数中，将 interrupt vector 转化为抽象中断层的中断信号 irq，调用中断信号 irq 对应的中断描述结构里面的 irq_handler_t

![img](https://static001.geekbang.org/resource/image/dd/13/dd492efdcf956cb22ce3d51592cdc113.png)

5. 从 CPU 收到中断向量开始分析。CPU 收到的中断向量定义在文件 arch/x86/include/asm/irq_vectors.h 中。CPU 能够处理的中断总共 256 个，用宏 NR_VECTOR 或者 FIRST_SYSTEM_VECTOR 表示。为了处理中断，CPU 硬件要求每一个 CPU 都有一个中断向量表，通过 load_idt 加载，里面记录着每一个中断对应的处理方法，这个中断向量表定义在文件 arch/x86/kernel/traps.c 中
6. 一个 CPU 可以处理的中断被分为几个部分，第一部分 0 到 31 的前 32 位是系统陷入或者系统异常，这些错误无法屏蔽，一定要处理。这些中断的处理函数在系统初始化的时候，在 start_kernel 函数中调用过 trap_init()，当前 32 个中断都用 set_intr_gate 设置完毕。在中断向量表 idt_table 中填完了之后，trap_init 单独调用 set_intr_gate 来设置 32 位系统调用的中断，在 trap_init 的最后，将 idt_table 放在一个固定的虚拟地址上。在 start_kernel 调用完毕 trap_init 之后，还会调用 init_IRQ() 来初始化其他的设备中断，最终会调用到 native_init_IRQ。从第 32 个中断开始，到最后 NR_VECTORS 为止，对于 used_vectors 中没有标记为 1 的位置，都会调用 set_intr_gate 设置中断向量表
7. 最终调用 do_IRQ，调用完毕后，就从中断返回。需要区分返回用户态还是内核态。这里会有一个机会触发抢占，do_IRQ 会根据中断向量 vector 得到对应的 irq_desc，然后调用 handle_irq。最终在__handle_irq_event_percpu 里面调用了 irq_desc 里每个 hander，这些 hander 在所有 action 列表中注册的，这才是设置的那个中断处理函数。如果返回值是 IRQ_HANDLED，就说明处理完毕；如果返回值是 IRQ_WAKE_THREAD 就唤醒线程

![img](https://static001.geekbang.org/resource/image/26/8f/26bde4fa2279f66098856c5b2b6d308f.png)

### 块设备

mknod 还是会创建在 /dev 路径下面，这一点和字符设备一样，块设备的打开往往不是直接调用设备文件的打开函数，而是调用 mount 来打开

1. /dev 路径下面是 devtmpfs 文件系统。这是块设备遇到的第一个文件系统。为这个块设备文件分配一个特殊的 inode，这点和字符设备也一样。只不过字符设备走 S_ISCHR 这个分支，对应 inode 的 file_operations 是 def_chr_fops；而块设备走 S_ISBLK 这个分支，对应的 inode 的 file_operations 是 def_blk_fops

2. 将这个块设备文件挂载到一个文件夹下面。如果这个块设备原来被格式化为一种文件系统的格式，例如 ext4，那调用的就是 ext4 相应的 mount 操作。这是块设备遇到的第二个文件系统，也是向这个块设备读写文件，需要基于的主流文件系统。ext4_mount->mount_bdev，mount_bdev 主要做了两件大事情

   * 第一：blkdev_get_by_path 根据 /dev/xxx 这个名字，找到相应的设备并打开它，blkdev_get_by_path做两件事

     * 调用lookup_bdev 根据设备路径 /dev/xxx 得到 block_device：lookup_bdev 这里的 pathname 是设备的文件名，例如 /dev/xxx是在 devtmpfs 文件系统中的，kern_path 可以在这个文件系统里面，一直找到它对应的 dentry。d_backing_inode 会获得 inode。这个 inode 就是那个 init_special_inode 生成的特殊 inode。bd_acquire 通过这个特殊的 inode，找到 struct block_device，bd_acquire 中最主要的就是调用 bdget，它的参数是特殊 inode 的 i_rdev。在 mknod 的时候，放的是设备号 dev_t，在 bdget 中，遇到了第三个文件系统，bdev 伪文件系统。bdget 函数根据传进来的 dev_t，在 blockdev_superblock 这个文件系统里面找到 inode。这个 inode 已经不是 devtmpfs 文件系统的 inode 了。所有表示块设备的 inode 都保存在伪文件系统 bdev 中，这些对用户层不可见，主要为了方便块设备的管理。Linux 将块设备的 block_device 和 bdev 文件系统的块设备的 inode，通过 struct bdev_inode 进行关联

     ```
     struct bdev_inode {
       struct block_device bdev;
       struct inode vfs_inode;
     };
     ```

     设备文件 /dev/xxx 在 devtmpfs 文件系统中，找到 devtmpfs 文件系统中的 inode，里面有 dev_t。通过 dev_t，在伪文件系统 bdev 中找到对应的 inode，然后根据 struct bdev_inode 找到关联的 block_device

     * 调用 blkdev_get打开这个设备，有一个磁盘 /dev/sda，既可以把它整个格式化成一个文件系统，也可以把它分成多个分区 /dev/sda1、 /dev/sda2，然后把每个分区格式化成不同的文件系统

     ![img](https://static001.geekbang.org/resource/image/85/76/85f4d83e7ebf2aadf7ffcd5fd393b176.png)

     struct gendisk 是用来描述整个设备的

     ```
     major 是主设备号
     first_minor 表示第一个分区的从设备号
     minors 表示分区的数目
     disk_name 给出了磁盘块设备的名称
     struct disk_part_tbl 结构里是一个 struct hd_struct 的数组，用于表示各个分区
     struct block_device_operations fops 指向对于这个块设备的各种操作
     struct request_queue queue 是表示在这个块设备上的请求队列
     ```

     ```
     struct hd_struct 是用来表示某个分区的，有两个实例，分别指向 /dev/sda1、 /dev/sda2
     较重要的成员变量保存了如下的信息：从磁盘的哪个扇区开始，到哪个扇区结束
     ```

     block_device 既可以表示整个块设备，也可以表示某个分区，如上block_device 有三个实例，分别指向 /dev/sda1、/dev/sda2、/dev/sda。block_device 的成员变量 bd_disk，指向的 gendisk 就是整个块设备。这三个实例都指向同一个 gendisk。bd_part 指向的某个分区的 hd_struct，bd_contains 指向的是整个块设备的 block_device。 __blkdev_get 函数中，先调用 get_gendisk，根据 block_device 获取 gendisk，关联的调用链：add_disk->device_add_disk->blk_register_region，将 dev_t 和 gendisk 关联起来

     * 如果 partno 为 0，打开的是整个设备，调用 disk_get_part，获取 gendisk 中的分区数组，然后调用 block_device_operations 里面的 open 函数打开设备
     * 如果 partno 不为 0，打开的是分区，获取整个设备的 block_device，赋值给变量 struct block_device *whole，然后调用递归 __blkdev_get，打开 whole 代表的整个设备，将 bd_contains 设置为变量 whole

     block_device_operations 就是在驱动层了。例如在 drivers/scsi/sd.c 里面，MODULE_DESCRIPTION(“SCSI disk (sd) driver”) 中，此时block_device 相应的成员变量该填的都填上了

   * 第二：sget 根据打开的设备文件，将 block_device填充 ext4 文件系统的 super_block，有了 ext4 文件系统的 super_block 之后，接下来对于文件的读写过程和之前一样

open一个块设备，涉及两个文件系统：devtmpfs和伪文件系统bdev。通过devtmpfs中的设备号dev_t在伪文件系统bdev中找到block_device，然后打开，打开后再将block_device设置到主流文件系统的super_block中。设置到主流文件系统的super_block后，就可以通过主流文件系统（如ext4）的file_operations对块设备进行操作了。

主要流程：

1. 所有的块设备被一个 map 结构管理从 dev_t 到 gendisk 的映射
2. 所有的 block_device 表示的设备或者分区都在 bdev 文件系统的 inode 列表中
3. mknod 创建出来的块设备文件在 devtemfs 文件系统里面，特殊 inode 里面有块设备号
4. mount 一个块设备上的文件系统，调用这个文件系统的 mount 接口
5. 通过按照 /dev/xxx 在文件系统 devtmpfs 文件系统上搜索到特殊 inode，得到块设备号
6. 根据特殊 inode 里面的 dev_t 在 bdev 文件系统里面找到 inode
7. 根据 bdev 文件系统上的 inode 找到对应的 block_device，根据 dev_t 在 map 中找到 gendisk，将两者关联
8. 找到 block_device 后打开设备，调用和 block_device 关联的 gendisk 里面的 block_device_operations 打开设备
9. 创建被 mount 的文件系统的 super_block

![img](https://static001.geekbang.org/resource/image/62/20/6290b73283063f99d6eb728c26339620.png)

##### 直接 I/O

generic_file_direct_write -> mapping->a_ops->direct_IO-> ext4_direct_IO，往设备层写入数据。__blockdev_direct_IO，有个参数 inode->i_sb->s_bdev。通过当前文件的 inode，得到 super_block。这个 super_block 中的 s_bdev，就是之前填进去的那个 block_device。接着调用do_blockdev_direct_IO->submit_page_section->dio_bio_submit->submit_bio 向块设备层提交数据。参数 struct bio 是将数据传给块设备的通用传输对象

##### 缓存 I/O

将数据从应用拷贝到内存缓存中，但并不执行真正的 I/O 操作。只将整个页或其中部分标记为脏。写操作由一个 timer 触发，才调用 wb_workfn 往硬盘写入页面。接下来的调用链为：wb_workfn->wb_do_writeback->wb_writeback->writeback_sb_inodes->__writeback_single_inode->do_writepages->mapping->a_ops->writepages，但实际调用的是 ext4_writepages，往设备层写入数据。有个比较重要的数据结构是 struct mpage_da_data。里面有文件的 inode、要写入的页的偏移量，还有 struct ext4_io_submit，里面有通用传输对象 bio。接下来的调用链为：mpage_prepare_extent_to_map->mpage_process_page_bufs->mpage_submit_page->ext4_bio_write_page->io_submit_add_bh，此时的 bio 还是空的，调用 io_submit_init_bio，初始化 bio。回到 ext4_writepages 中。在 bio 初始化完之后，调用 ext4_io_submit，提交 I/O。又调用 submit_bio，向块设备层传输数据。

##### 向块设备层提交请求

不管是直接 I/O，还是缓存 I/O，最后都到了 submit_bio 里面，generic_make_request，先是获取一个请求队列 request_queue，然后调用make_request_fn。对于 struct block_device 结构和 struct gendisk 结构，每个块设备都有一个请求队列 struct request_queue，用于处理上层发来的请求。在每个块设备的驱动程序初始化的时候，会生成一个 request_queue。在request_queue 上，首先是有一个链表 list_head，保存请求 request。每个 request 包括一个链表的 struct bio，在 bio 中，bi_next 是链表中的下一项，struct bio_vec 指向一组页面。在请求队列 request_queue 上，除了 make_request_fn 用于生成 request；另一个 request_fn用于处理 request

![img](https://static001.geekbang.org/resource/image/3c/0e/3c473d163b6e90985d7301f115ab660e.jpeg)

##### 块设备的初始化

以 scsi 驱动为例。在初始化设备驱动的时候，调用 scsi_alloc_queue，把 request_fn 设置为 scsi_request_fn。调用 blk_init_allocated_queue->blk_queue_make_request，把 make_request_fn 设置为 blk_queue_bio。除了初始化 make_request_fn 函数，还有初始化 I/O 的电梯算法，默认 iosched_cfq：

* struct elevator_type elevator_noop：最简单的 IO 调度算法，它将 IO 请求放入到一个 FIFO 队列中，然后逐个执行这些 IO 请求
* struct elevator_type iosched_deadline：保证每个 IO 请求在一定的时间内一定要被服务到，以此来避免某个请求饥饿
* struct elevator_type iosched_cfq：完全公平调度算法。所有的请求会在多个队列中排序。同一个进程的请求，总是在同一队列中处理

##### 请求提交与调度

回到 generic_make_request 函数中。它处理两大逻辑：获取一个请求队列 request_queue 和调用这个队列的 make_request_fn 函数。调用队列的 make_request_fn 函数，其实就是调用 blk_queue_bio，首先调用 elv_merge 来判断，当前 bio 请求是否能够和目前已有的 request 合并起来，成为同一批 I/O 操作，从而提高读取和写入的性能。判断标准和 struct bio 的成员 struct bvec_iter 有关，里面有两个变量，一个是起始磁盘簇 bi_sector，另一个是大小 bi_size，elv_merge 尝试了三次合并

* 第一次，判断和上一次合并的 request 能不能再次合并
* 第二次，调用 elv_rqhash_find 然后按照 bio 的起始地址查找 request，看有没有能够合并的
* 第三次，调用 elevator_merge_fn 试图合并，对于 iosched_cfq，调用 cfq_merge->cfq_find_rq_fmerge -> elv_rb_find 

如果没有办法合并，就调用 get_request创建一个新的 request，调用 blk_init_request_from_bio，将 bio 放到新的 request 里面，然后调用 add_acct_request，把新的 request 加到 request_queue 队列中。generic_make_request 的逻辑。对于写入的数据来讲，其实仅仅是将 bio 请求放在请求队列上

##### 请求的处理

设备驱动程序往设备里面写，调用的是请求队列 request_queue 的另外一个函数 request_fn。对于 scsi 设备来讲，调用的是 scsi_request_fn，这里面是一个 for 无限循环，从 request_queue 中读取 request，然后封装更加底层的指令，给设备控制器下指令，实施真正的 I/O 操作

![img](https://static001.geekbang.org/resource/image/c9/3c/c9f6a08075ba4eae3314523fa258363c.png)

