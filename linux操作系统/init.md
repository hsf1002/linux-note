#### X86体系

计算机的工作模式

![img](https://static001.geekbang.org/resource/image/fa/9b/fa6c2b6166d02ac37637d7da4e4b579b.jpeg)

CPU结构基于英特尔的 8086，因此称为 x86 架构

```
型号	总线位宽	地址位		寻址空间
8080	8		16			64K(2^16)
8086	16		20			1M
8088	8		20			1M
80386	32		32			4G
```

![img](https://static001.geekbang.org/resource/image/3a/23/3afda18fc38e7e53604e9ebf9cb42023.jpeg)

CPU与内存传数据，主要有两类数据，地址数据和真正的数据

地址总线：位数决定了能访问的地址范围

数据总线：位数决定了一次能保存多少位数据

![img](https://static001.geekbang.org/resource/image/54/8a/548dfd163066d061d1e882c73e7c2b8a.jpg)

### 16位寄存器

8个16位通用寄存器：AX、BX、CX、DX、SP、BP、SI、DI

AX、BX、CX、DX可以分为AH、AL、BH、BL、CH、CL、DH、DL

IP：指令指针寄存器（Instruction Pointer Register)，指向代码段中下一条指令的位置

为了切换进程，有四个16位段寄存器：

CS：代码段寄存器（Code Segment Register），通过它可以找到代码在内存中的位置

DS：数据段的寄存器（Data Register），通过它可以找到数据在内存中的位置

SS：栈寄存器（Stack Register），凡是与函数调用相关的操作，都与栈紧密相关

ES：附加段寄存器（Extra Register）

如果运算中需要加载内存中的数据，需要通过 DS 找到内存中的数据，起始地址+偏移量，加载到通用寄存器中

在 CS 和 DS 中都存放着一个段的起始地址。代码段的偏移量会放在通用寄存器中

起始地址CS和DS，偏移量IP和通用寄存器，都是16位，而地址总线是20位，需要把CS和DS左移4位，变成20位，加上16位的偏移量。8086CPU，最多只能访问 1M 的内存空间，还要分成多个段，每个段最多 64K。在 32 位处理器中，有 32 根地址总线，可以访问 2^32=4G 的内存

### 32位寄存器

8个16位通用寄存器，以及IP，依旧放在低16位，可以兼容32位，改动比较大的是段寄存器

CS、SS、DS、ES 仍然是 16 位的，但是不再是段的起始地址。段的起始地址放在内存的某个地方。这个地方是一个表格，里面是真正的段的起始地址。而段寄存器里面保存的是在这个表格中的哪一项，称为选择子（Selector）。将一个从段寄存器直接拿到的段起始地址，就变成了先间接地从段寄存器找到表格中的一项，再从表格中的一项中拿到段起始地址。

32 位的系统架构下，我们将前一种模式称为实模式（Real Pattern），后一种模式称为保护模式（Protected Pattern）。当系统刚刚启动的时候，CPU 是处于实模式的，这个时候和原来的模式是兼容的。也就是说，哪怕你买了 32 位的 CPU，也支持在原来的模式下运行，只不过快了一点而已

![img](https://static001.geekbang.org/resource/image/e2/76/e2e92f2239fe9b4c024d300046536d76.jpeg)

### BIOS

![img](https://static001.geekbang.org/resource/image/5f/fc/5f364ef5c9d1a3b1d9bb7153bd166bfc.jpeg)

ROM会固化一些初始化的程序，如BIOS。实模式只有 1MB 内存寻址空间。加电, 重置 CS 为 0xFFFF , IP 为 0x0000, 第一条指令指向0xFFFF0，BIOS 做以下三件事:

- 检查硬件
- 提供基本输入(中断)输出(显存映射)服务
- 加载 MBR 到内存(0x7c00)

BIOS的界面会有一个启动盘，一般在第一个扇区，占512个字节，以0xAA55结束，启动盘是Grub2放置在这的

Grub2：Grand Unified Bootloaer Version 2：

```
grub2-mkconfig -o /boot/grub2/grub.cfg
```

MBR：启动盘第一个扇区(512字节, 由 Grub2 写入 boot.img 镜像)，BIOS 完成任务后，会将 boot.img 从硬盘加载到内存中的 0x7c00 来运行

### Bootloader

![img](https://static001.geekbang.org/resource/image/2b/6a/2b8573bbbf31fc0cb0420e32d07b196a.jpeg)

boot.img 加载 Grub2 的 core.img 镜像；core.img 包括 diskroot.img(diskboot.S)、 lzma_decompress.img(startup_raw.S)、kernel.img(startup.S) 以及其他模块；boot.img 先加载运行 diskroot.img, 再由 diskroot.img 加载 core.img 的其他内容；diskroot.img 解压运行 lzma_decompress.img，由lzma_decompress.img 切换到保护模式，kernel.img(grub 的内核，选择操作系统)，最后是各个模块对应的映像

- lzma_decompress.img切换到保护模式需要做以下三件事:
  - 启用分段, 辅助进程管理
  - 启动分页, 辅助内存管理
  - 打开其他地址线，第 21 根要起作用，打开Gate A20
- lzma_decompress.img解压运行kernel.img 做以下四件事:
  - 解析 grub.conf 文件
  - 选择操作系统
  - 例如选择 linux16, 会先读取内核头部数据进行检查, 检查通过后加载完整系统内核
  - 启动系统内核 grub_command_execute (“boot”, 0, 0)

### 内核初始化

`start_kernel()` 函数(位于 init/main.c), 初始化做三件事

- 创建样板进程, 及各个模块初始化
- 创建管理/创建用户态进程的进程
- 创建管理/创建内核态进程的进程

### 0号进程

样板进程, 即第一个进程. `set_task_stack_end_magic(&init_task)`，定义是 `struct task_struct init_task = INIT_TASK(init_task)`，这是唯一一个没有通过fork 或者 kernel_thread 产生的进程，是进程列表的第一个

各个其他模块初始化包括：

- 初始化中断, `trap_init()`. 系统调用也是通过发送中断进行, 由 `set_system_intr_gate()` 完成.
- 初始化内存管理模块, `mm_init()`
- 初始化进程调度模块, `sched_init()`
- 初始化基于内存的文件系统 rootfs, `vfs_caches_init()`
  - VFS(虚拟文件系统)将各种文件系统抽象成统一接口
- 调用 `rest_init()` 完成其他初始化工作

### 1号进程

![img](https://static001.geekbang.org/resource/image/2b/42/2b53b470673cde8f9d8e2573f7d07242.jpg)

![img](https://static001.geekbang.org/resource/image/71/e6/71b04097edb2d47f01ab5585fd2ea4e6.jpeg)

![img](https://static001.geekbang.org/resource/image/d2/14/d2fce8af88dd278670395ce1ca6d4d14.jpg)

创建管理/创建用户态进程的进程，是所有用户态进程的祖先，这是第一个用户态进程

- ` rest_init()` 通过 `kernel_thread(kernel_init, NULL, CLONE_FS)` 创建1号进程(工作在用户态)
- 权限管理
  - x86 提供 4个 Ring 分层权限，Ring0：内核，Ring1：设备驱动，Ring2：设备驱动，Ring3：应用
  - 操作系统利用: Ring0-内核态(访问核心资源); Ring3-用户态(普通程序)
- 用户态调用系统调用: 用户态-系统调用-保存寄存器-内核态执行系统调用-恢复寄存器-返回用户态
- 新进程执行 kernel_init 函数, 先运行 ramdisk 的 /init 程序(位于内存中)
  - 首先加载 ELF 文件（Executable and Linkable Format，可执行与可链接格式）
  - 设置用于保存用户态寄存器的结构体
  - 进入内核态
  - /init 加载存储设备的驱动
  - 恢复寄存器。CS 和指令指针寄存器 IP指向用户态下一个要执行的语句，DS 和函数栈指针 SP 指向用户态函数栈的栈顶
  - 下一条指令，就从用户态开始运行
- kernel_init 函数启动存储设备文件系统上的 init

ramdisk：由于 init 程序是在文件系统上的，文件系统一定是在一个存储设备上的，例如硬盘。Linux 访问存储设备，要有驱动才能访问。如果所有市面上的存储系统的驱动都默认放进内核，内核就太大了，基于内存的文件系统。内存访问不需要驱动。ramdisk 是根文件系统。开始运行 ramdisk 上的 /init。等它运行完了就已经在用户态了。/init 这个程序会先根据存储系统的类型加载驱动，有了驱动就可以设置真正的根文件系统了。有了真正的根文件系统，ramdisk 上的 /init 会启动文件系统上的 init。接下来就是各种系统的初始化。启动系统的服务，启动控制台，用户就可以登录进来了。rest_init 的第一大事情才完成。仅仅形成了用户态所有进程的祖先。rest_init 第二大事情就是第三个进程，就是 2 号进程

### 2号进程

创建管理/创建内核态进程的进程，是所有内核态进程的祖先

- `rest_init()` 通过 `kernel_thread(kthreadd, NULL, CLONE_FS | CLONE_FILES)` 创建 2号进程(工作在内核态)
- `kthreadd` 负责所有内核态线程的调度和管理

### glibc 

将系统调用封装成更友好的接口，用户进程调用 open 函数的流程：

- glibc 的 syscal.list 列出 glibc 函数对应的系统调用

  ```
  # File name Caller  Syscall name    Args    	Strong name 	Weak names
  open		-		open			Ci:siv	__libc_open __open 	open
  ```

- glibc 的脚本 make_syscall.sh 根据 syscal.list 生成对应的宏定义(函数映射到系统调用)

  ```
  #define SYSCALL_NAME open
  ```

- glibc 的 syscal-template.S 使用这些宏, 定义了系统调用的调用方式(也是通过宏)

  ```
  T_PSEUDO (SYSCALL_SYMBOL, SYSCALL_NAME, SYSCALL_NARGS)
      ret
  T_PSEUDO_END (SYSCALL_SYMBOL)
  
  #define T_PSEUDO(SYMBOL, NAME, N)		PSEUDO (SYMBOL, NAME, N)
  ```

- 其中会调用 DO_CALL (也是一个宏), 32位与 64位实现不同

32位 DO_CALL (位于 i386 目录下 sysdep.h)：

- 将调用参数放入寄存器中, 由系统调用名得到系统调用号, 放入 eax

- 执行 ENTER_KERNEL(一个宏), 对应 int $0x80 触发软中断, 进入内核

- 调用软中断处理函数 entry_INT80_32(内核启动时, 由 trap_init() 配置)

  ```
  set_system_intr_gate(IA32_SYSCALL_VECTOR, entry_INT80_32);
  ```

- entry_INT80_32 将用户态寄存器存入 pt_regs 中(保存现场以及系统调用参数), 调用 do_syscall_32_iraq_on 

- do_syscall_32_iraq_on 从 pt_regs 中取系统调用号(eax), 从系统调用表得到对应实现函数, 取 pt_regs 中存储的参数, 调用系统调用

- entry_INT80_32 调用 INTERRUPT_RUTURN(一个宏)对应 iret 指令, 系统调用结果存在 pt_regs 的 eax 位置, 根据 pt_regs 恢复用户态进程

  ```
  #define INTERRUPT_RETURN                iret
  ```

![img](https://static001.geekbang.org/resource/image/56/06/566299fe7411161bae25b62e7fe20506.jpg)

64位 DO_CALL (位于 x86_64 目录下 sysdep.h)：

- 通过系统调用名得到系统调用号, 存入 rax; 不同于中断, 执行的是 syscall 指令，而且传递参数的寄存器也变了

- syscall 使用了MSR(特殊模块寄存器), 辅助完成某些功能(包括系统调用)

- trap_init() 会调用 cpu_init->syscall_init 设置该寄存器

  ```
  wrmsrl(MSR_LSTAR, (unsigned long)entry_SYSCALL_64);
  ```

- syscall 从 MSR 寄存器中拿出函数地址进行调用, 即调用 entry_SYSCALL_64

- entry_SYSCALL_64 先保存用户态寄存器到 pt_regs 中

- 调用 entry_SYSCALL64_slow_pat->do_syscall_64

- do_syscall_64 从 rax 取系统调用号, 从系统调用表得到对应实现函数, 取 pt_regs 中存储的参数, 调用系统调用

- 返回执行 USERGS_SYSRET64(一个宏), 对应执行 swapgs 和 sysretq 指令; 系统调用结果存在 pt_regs 的 ax 位置, 根据 pt_regs 恢复用户态进程

  ```
  #define USERGS_SYSRET64				\
  	swapgs;					\
  	sysretq;
  ```

![img](https://static001.geekbang.org/resource/image/1f/d7/1fc62ab8406c218de6e0b8c7e01fdbd7.jpg)

无论是 32 位，还是 64 位，都会回到系统调用表 sys_call_table，而32 位和 64 位的系统调用号是不一样的：

- 32位 定义在 arch/x86/entry/syscalls/syscall_32.tbl 

  ```
  5	i386	open			sys_open  compat_sys_open
  ```

- 64位 定义在 arch/x86/entry/syscalls/syscall_64.tbl

  ```
  2	common	open			sys_open
  ```

- syscall_*.tbl 内容包括: 系统调用号, 系统调用名, 内核实现函数名(以 sys 开头)

- 内核实现函数的声明: include/linux/syscall.h

- 内核实现函数的实现: 某个 .c 文件, 例如 sys_open 的实现在 fs/open.c

  - .c 文件中, 以宏的方式替代函数名, 用多层宏构建函数头

- 编译过程中, 通过 syscall_32.tbl和syscall_64.tbl  生成 unistd_32.h 和 unistd_64.h文件

  - unistd_32.h 和 unistd_64.h包含系统调用与实现函数的对应关系

- syscall_*.h include 了 unistd_*.h 头文件, 并定义了系统调用表(数组)

![img](https://static001.geekbang.org/resource/image/86/a5/868db3f559ad08659ddc74db07a9a0a5.jpg)