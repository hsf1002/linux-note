# linux操作系统



#### X86体系

CPU结构基于英特尔的 8086，因此称为 x86 架构

```
型号	总线位宽	地址位		寻址空间
8080	8		16			64K(2^16)
8086	16		20			1M
8088	8		20			1M
80386	32		32			4G
```

CPU与内存传数据，主要有两类数据，地址数据和真正的数据

地址总线：位数决定了能访问的地址范围

数据总线：位数决定了一次能保存多少位数据

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

起始地址CS和DS，偏移量IP和通用寄存器，都是16位，而地址总线是20位，需要把CS和DS左移4位，变成20位，加上16位的偏移量。8086CPU，最多只能访问 1M 的内存空间，还要分成多个段，每个段最多 64K

### 32位寄存器

8个16位通用寄存器，以及IP，依旧放在低16位，可以兼容32位，改动比较大的是段寄存器

CS、SS、DS、ES 仍然是 16 位的，但是不再是段的起始地址。段的起始地址放在内存的某个地方。这个地方是一个表格，里面是真正的段的起始地址。而段寄存器里面保存的是在这个表格中的哪一项，称为选择子（Selector）。将一个从段寄存器直接拿到的段起始地址，就变成了先间接地从段寄存器找到表格中的一项，再从表格中的一项中拿到段起始地址。

32 位的系统架构下，我们将前一种模式称为实模式（Real Pattern），后一种模式称为保护模式（Protected Pattern）。当系统刚刚启动的时候，CPU 是处于实模式的，这个时候和原来的模式是兼容的。也就是说，哪怕你买了 32 位的 CPU，也支持在原来的模式下运行，只不过快了一点而已

### BIOS

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

boot.img 加载 Grub2 的 core.img 镜像；core.img 包括 diskroot.img(diskboot.S)、 lzma_decompress.img(startup_raw.S)、kernel.img(startup.S) 以及其他模块；boot.img 先加载运行 diskroot.img, 再由 diskroot.img 加载 core.img 的其他内容；diskroot.img 解压运行 lzma_decompress.img、kernel.img(grub 的内核)，最后是各个模块对应的映像， 由lzma_decompress.img 切换到保护模式

- lzma_decompress.img切换到保护模式需要做以下三件事:
  - 启用分段, 辅助进程管理
  - 启动分页, 辅助内存管理
  - 打开其他地址线

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

创建管理/创建用户态进程的进程，是所有内核态进程的祖先，这是第一个用户态进程

- ` rest_init()` 通过 `kernel_thread(kernel_init, NULL, CLONE_FS)` 创建1号进程(工作在用户态)

- 权限管理
  - x86 提供 4个 Ring 分层权限，Ring0：内核，Ring1：设备驱动，Ring2：设备驱动，Ring3：应用
  - 操作系统利用: Ring0-内核态(访问核心资源); Ring3-用户态(普通程序)
- 用户态调用系统调用: 用户态-系统调用-保存寄存器-内核态执行系统调用-恢复寄存器-返回用户态
- 新进程执行 kernel_init 函数, 先运行 ramdisk 的 /init 程序(位于内存中)
  - 首先加载 ELF 文件（Executable and Linkable Format，可执行与可链接格式）
  - 设置用于保存用户态寄存器的结构体
  - 返回进入用户态
  - /init 加载存储设备的驱动，这时就切到用户态开始运行了
- kernel_init 函数启动存储设备文件系统上的 init

ramdisk：基于内存的文件系统。内存访问不需要驱动。这个时候，ramdisk 是根文件系统。开始运行 ramdisk 上的 /init。等它运行完了就已经在用户态了。/init 这个程序会先根据存储系统的类型加载驱动，有了驱动就可以设置真正的根文件系统了。有了真正的根文件系统，ramdisk 上的 /init 会启动文件系统上的 init。接下来就是各种系统的初始化。启动系统的服务，启动控制台，用户就可以登录进来了。rest_init 的第一大事情才完成。仅仅形成了用户态所有进程的祖先。rest_init 第二大事情就是第三个进程，就是 2 号进程

### 2号进程

创建管理/创建内核态进程的进程，是所有内核态进程的祖先

- `rest_init()` 通过 `kernel_thread(kthreadd, NULL, CLONE_FS | CLONE_FILES)` 创建 2号进程(工作在内核态)
- `kthreadd` 负责所有内核态线程的调度和管理

### glibc 

对于 64 位操作系统，找到 unistd_64.h 文件，里面对于系统调用的定义。将系统调用封装成更友好的接口，用户进程调用 open 函数的流程：

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



无论是 32 位，还是 64 位，都会回到系统调用表 sys_call_table：

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

### ELF文件的分类

编译成 ELF 格式的二进制文件, 有三种格式(可重定位 .o 文件; 可执行文件; 共享对象文件 .so)

- 可重定位 .o 文件(ELF 第一种格式)

  - .h + .c 文件, 编译得到**可重定位** .o 文件 

  - .o 文件由: ELF 头, 多个节(section), 节头部表组成(每个节的元数据); 节表的位置和纪录数由 ELF 头给出

    ```
    section包括：
    .text：放编译好的二进制可执行代码
    .data：已经初始化好的全局变量
    .rodata：只读数据，例如字符串常量、const 的变量
    .bss：未初始化全局变量，运行时会置 0
    .symtab：符号表，记录的则是函数和变量
    .strtab：字符串表、字符串常量和变量名
    ```

  - .o 文件只是程序部分代码片段

  - .rel.text 和 .rel.data 标注了哪些函数/变量需要重定位

  - 要函数可被调用, 要以库文件的形式存在, 最简单是创建静态链接库 .a 文件(Archives)，通过 ar 创建静态链接库

    ```
    ar cr libstaticprocess.a process.o
    
    ```

     通过 gcc 提取库文件中的 .o 文件, 链接到程序中

    ```
    gcc -o staticcreateprocess createprocess.o -L. -lstaticprocess
    
    -L 表示在当前目录下找.a 文件，-lstaticprocess 会自动补全文件名，比如加前缀 lib，后缀.a，变成 libstaticprocess.a，找到这个.a 文件后，将里面的 process.o 取出来，和 createprocess.o 做一个链接，形成二进制执行文件 staticcreateprocess
    
    ```

  - 链接合并后, 就可以定位到函数/数据的位置, 形成可执行文件

- 可执行文件(ELF 第二种格式)

  - 链接合并后, 形成可执行文件，这个格式和.o 文件大致相似，还是分成一个个的 section，并且被节头表描述。只不过这些 section 是多个.o合并过的

  - 同样包含: ELF 头, 多个节, 节头部表; 另外还有段头表(包含段的描述, p_vaddr 段加载到内存的虚拟地址)

    ```
    section包括：
    代码段：
    .text：放编译好的二进制可执行代码
    .rodata：只读数据，例如字符串常量、const 的变量
    
    数据段：
    .data：已经初始化好的全局变量
    .bss：未初始化全局变量，运行时会置 0
    
    不加载到内存：
    .symtab：符号表，记录的则是函数和变量
    .strtab：字符串表、字符串常量和变量名
    ```

  - ELF 头中有 e_entry , 指向程序入口的虚拟地址

  - 静态链接库一旦链接进去，代码和变量的 section 都合并了，因而程序运行的时候，就不依赖于这个库是否存在。但是这样相同的代码段，如果被多个程序使用的话，在内存里面就有多份，而且一旦静态链接库更新了，如果二进制执行文件不重新编译，也不随着更新

- 共享对象 .so 文件(Shared Libraries，ELF 第三种格式)

  - 静态链接库合并进可执行文件, 但是多个进程不能共享

  - 动态链接库-链接了动态链接库的程序, 仅包含对该库的引用(且只保存名称),而不包含其代码

    ```
    gcc -o dynamiccreateprocess createprocess.o -L. -ldynamicprocess
    
    通过 gcc 创建和链接, 运行时, 先找到动态链接库(默认在 /lib 和 /usr/lib 找),找不到就会报错
    
    设定 LD_LIBRARY_PATH环境变量，程序运行时会在此环境变量指定的文件夹下寻找动态链接库
    
    export LD_LIBRARY_PATH=.
    ```

  - ELF的section增加了 .interp 段, 里面是 ld_linux.so (动态链接器)

  - 增加了两个节 .plt(Procedure Linkage Table：过程链接表)和 .got.plt(Global Offset Table：全局偏移表)

  - 一个动态链接函数对应 plt 中的一项 plt[x], plt[x] 中是代理代码, 调用 got 中的一项 got[y]

  - 起始, got 没有动态链接函数的地址, 都指向 plt[0], plt[0] 又调用 got[2], got[2]指向 ld_linux.so

  - ld_linux.so 找到加载到内存的动态链接函数的地址, 并将地址存入 got[y]

- 加载 ELF 文件到内存

  - 系统调用 exec 最终调用 load_elf_binary(do_execve->do_execveat_common->exec_binprm->search_binary_handler)
  - exec 是一组函数
    - 包含 p: 在 PATH 中找程序，如 execvp, execlp
    - 不包含 p: 需提供全路径
    - 包含 v: 以数字接收参数，如 execv, execvp, execve
    - 包含 l: 以列表接收参数，如 execl, execlp, execle
    - 包含 e: 以数字接收环境变量，如 execve, execle

- 进程树

  所有的进程都是从父进程 fork 过来的，祖宗进程就是系统启动的 init 进程，系统启动之后，init 进程会启动很多的 daemon 进程，为系统运行提供服务，然后启动 getty，让用户登录

  - ps -ef: 用户进程不带中括号, 内核进程带中括号
  - 用户进程祖先(1号进程, systemd); 内核进程祖先(2号进程, kthreadd)
  - tty ? 一般表示后台服务

- 分析工具
  - readelf 工具用于分析 ELF 的信息
  - objdump工具用来显示二进制文件的信息
  - hexdump 工具用来查看文件的十六进制编码
  - nm 工具用来显示关于指定文件中符号的信息

### 多线程并行

多进程缺点：创建进程占用资源多; 进程间通信需拷贝内存, 不能共享

- 线程相关操作

  - pthread_exit(A), A 是线程退出的返回值
  - pthread_attr_t 线程属性, 用辅助函数初始化并设置值; 用完需要销毁
  - pthread_create 创建线程, 四个参数(线程对象, 属性, 运行函数, 运行参数)
  - pthread_join 获取线程退出返回值, 多线程依赖 libpthread.so
  - 一个线程退出, 会发送信号给 其他所有同进程的线程

- 线程中有三类数据

  - 线程栈本地数据, 栈大小默认 8MB(通过命令 ulimit -a 查看， ulimit -s 修改); 线程栈之间有保护间隔, 若误入会引发段错误

    ```
    修改线程栈的大小：
    int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize);
    ```

  - 进程共享的全局数据，若多个线程一起修改，会有问题

  - 线程级别的全局变量(线程私有数据); key 所有线程都可以访问, 可填入各自的值(同名不同值的全局变量)，等到线程退出的时候，就会调用析构函数释放 value

  - ```
    创建：
    int pthread_key_create(pthread_key_t *key, void (*destructor)(void*))
    
    设置：
    int pthread_setspecific(pthread_key_t key, const void *value)
    
    获取：
    void *pthread_getspecific(pthread_key_t key
    ```

- 数据保护

  - Mutex(互斥)，lock(没抢到则阻塞)/trylock(没抢到则返回错误码)

    ```
    初始化：
    pthread_mutex_init(&g_task_lock, NULL);
    
    加锁：
    pthread_mutex_lock(&g_task_lock);
    
    解锁：
    pthread_mutex_unlock(&g_task_lock)
    
    销毁：
    pthread_mutex_destroy(&g_task_lock);
    ```

    

  - 条件变量(通知), 收到通知, 还是要抢锁(由 wait 函数执行); 因此条件变量与互斥锁配合使用

    ```
    初始化：
    pthread_cond_init(&g_task_cv, NULL);
    
    通知所有线程：
    pthread_cond_broadcast(&g_task_cv);
    
    等待，收到通知就退出
    pthread_cond_wait(&g_task_cv, &g_task_lock);
    
    销毁：
    pthread_cond_destroy(&g_task_cv);
    
    ```

#### 内核任务

内核中进程, 线程统一为任务, 由 taks_struct 表示，它是一个链表

- task_struct 中包含: 任务ID; 任务状态; 信号处理相关字段; 调度相关字段; 亲缘关系; 权限相关; 运行统计; 内存管理; 文件与文件系统; 内核栈

- 任务 ID; 包含 pid, tgid 和 \*group_leader

  - pid(process id, 线程的id); tgid(thread group id, 所属进程[主线程]的id); group_leader 指向 tgid 的结构体
  - 通过对比 pid 和 tgid 是否相同可判断是进程还是线程

- 信号处理, 包含阻塞暂不处理、等待处理、正在处理的信号

  - 信号处理函数默认使用用户态的函数栈, 也可以开辟新的栈专门用于信号处理, 由 sas_ss_xxx 指定
  - 通过 pending/shared_pending 区分进程和线程的信号

- 任务状态; 包含 state; exit_state; flags

  - 准备运行状态 TASK_RUNNING

  - 睡眠状态：可中断; 不可中断; 可杀

    - 可中断 TASK_INTERRUPTIBLE，浅睡眠，收到信号要被唤醒，只不过唤醒后，不是继续刚才的操作，而是进行信号处理

    - 不可中断 TASK_UNINTERRUPTIBLE，深睡眠，一旦等待的IO操作无法完成，则收到信号不会被唤醒, 自然不能被kill, 只能重启

    - 可杀 TASK_KILLABLE, 可以响应致命信号, 由不可中断与 TASK_WAKEKILL 组合

      ```
      #define TASK_KILLABLE           (TASK_WAKEKILL | TASK_UNINTERRUPTIBLE)
      
      ```

  - 停止状态 TASK_STOPPED, 由信号 SIGSTOP, SIGTTIN, SIGTSTP 与 SIGTTOU 触发进入

  - 调试跟踪 TASK_TRACED， 被 debugger 等进程监视时进入

  - 结束状态(包含 exit_state)

    - EXIT_ZOMBIE, 父进程还没有使用 wait()等系统调用获知它的终止信息，此时就是僵尸进程
    - EXIT_DEAD, 最终状态

  - 其他的标志则放到flags

    - PF_EXISTING 表示正在退出，在函数 find_alive_thread 中，找活着的线程时，遇到有这个 flag 的，就直接跳过
    - PF_VCPU 表示运行在虚拟 CPU 上
    - PF_FORKNOEXEC表示 fork 完了，还没有 exec，在 \_do_fork 函数里设置，exec 函数中清除

- 进程调度; 包含 是否在运行队列; 优先级; 调度策略; 可以使用那些 CPU 等信息

- 运行统计信息, 包含用户/内核态运行时间; 上/下文切换次数; 启动时间等

  ```
  u64				utime;// 用户态消耗的 CPU 时间
  u64				stime;// 内核态消耗的 CPU 时间
  unsigned long	nvcsw;//  自愿 (voluntary) 上下文切换计数
  unsigned long	nivcsw;// 非自愿 (involuntary) 上下文切换计数
  u64				start_time;		// 进程启动时间，不包含睡眠时间
  u64				real_start_time;// 进程启动时间，包含睡眠时间
  ```

- 进程亲缘关系

  ```
  struct task_struct __rcu *real_parent; 	/* real parent process */
  struct task_struct __rcu *parent; 		/* recipient of SIGCHLD, wait4() reports */
  struct list_head children;      /* list of my children */
  struct list_head sibling;       /* linkage in my parent's children list */
  ```

  - 拥有同一父进程的所有进程具有兄弟关系
  - children 表示链表的头部，链表中的所有元素都是它的子进程
  - sibling 用于把当前进程插入到兄弟链表中
  - parent 指向的父进程接收进程结束信号
  - real_parent 和 parent 通常一样; 但在 bash 中用 GDB 调试程序时, GDB 是 parent, bash 是 real_parent

- 进程权限, 包含 real_cred 指针(谁能操作我); cred 指针(我能操作谁)

  ```
  /* Objective and real subjective task credentials (COW): */
  const struct cred __rcu         *real_cred;	// 谁能操作我
  /* Effective (overridable) subjective task credentials (COW): */
  const struct cred __rcu         *cred;		// 我能操作谁
  ```

  ```
  struct cred {
  ......
          kuid_t          uid;            /* real UID of the task */
          kgid_t          gid;            /* real GID of the task */
          kuid_t          suid;           /* saved UID of the task */
          kgid_t          sgid;           /* saved GID of the task */
          kuid_t          euid;           /* effective UID of the task */
          kgid_t          egid;           /* effective GID of the task */
          kuid_t          fsuid;          /* UID for VFS ops */
          kgid_t          fsgid;          /* GID for VFS ops */
  ......
          kernel_cap_t    cap_inheritable; /* caps our children can inherit */
          kernel_cap_t    cap_permitted;  /* caps we're permitted */
          kernel_cap_t    cap_effective;  /* caps we can actually use */
          kernel_cap_t    cap_bset;       /* capability bounding set */
          kernel_cap_t    cap_ambient;    /* Ambient capability set */
  ......
  } __randomize_layout;
  ```

  - cred 结构体中标明多组用户和用户组 id

  - uid/gid(哪个用户的进程启动我，权限审核不比较这两个)

  - euid/egid(按照哪个用户审核权限, 操作消息队列, 共享内存等，真正起作用的用户和组)

  - fsuid/fsgid(文件操作时审核)

  - 一般说来，fsuid、euid，和 uid 是一样的，fsgid、egid，和 gid 也是一样的。因为谁启动的进程，就应该审核启动的用户到底有没有这个权限

  - 通过 chmod u+s program, 给程序设置 set-user-id 标识位, 运行时程序将进程 euid/fsuid 改为程序文件所有者 id，而program的实际所有者保存在suid/sgid中

  - suid/sgid 可以用来保存 id, 进程可以通过 setuid 更改 uid

  - capability 机制, 以细粒度赋予普通用户部分高权限 (capability.h 列出了权限)

    ```
    #define CAP_CHOWN            0
    #define CAP_KILL             5
    #define CAP_NET_BIND_SERVICE 10
    #define CAP_NET_RAW          13
    #define CAP_SYS_MODULE       16
    #define CAP_SYS_RAWIO        17
    #define CAP_SYS_BOOT         22
    #define CAP_SYS_TIME         25
    #define CAP_AUDIT_READ       37
    #define CAP_LAST_CAP         CAP_AUDIT_READ
    
    ```

    - cap_permitted 表示进程的权限
    - cap_effective 实际起作用的权限, cap_permitted 范围可大于 cap_effective，一个进程在必要的时候放弃某些权限，更加安全
    - cap_inheritable 若可执行文件的扩展属性设置了该权限，表示可被继承, 在 exec 执行时继承的权限集合, 并加入 cap_permitted 中(但非 root 用户不会保留 cap_inheritable 集合)
    - cap_bset(capability bounding set)所有进程保留的权限(限制只用一次的功能)，如系统启动以后，将加载内核模块的权限去掉，那所有进程都不能加载内核模块。即便这台机器被攻破，也做不了太多有害的事情
    - cap_ambient 比较新加入内核的，就是为了解决 cap_inheritabl鸡肋的问题 ，exec 时, 并入 cap_permitted 和 cap_effective 中

- 内存管理: mm_struct

  ```
  struct mm_struct                *mm;
  struct mm_struct                *active_mm;
  ```

- 文件与文件系统: 打开的文件, 文件系统相关数据结构

  ```
  /* Filesystem information: */
  struct fs_struct                *fs;
  /* Open file information: */
  struct files_struct             *files;
  ```

### 用户态函数栈

- 用户态/内核态切换如何关联：

```
struct thread_info		thread_info;
void  *stack;
```

- 用户态函数栈

  - 通过 JMP + 参数 + 返回地址 调用函数

  - 栈内存空间从高到低增长
  - 32位栈结构: 栈帧包含 前一个帧的 EBP + 局部变量 + N个参数 + 返回地址
    - ESP: 栈顶指针，push和pop操作后会自动调整
    - EBP: 栈基指针(栈帧最底部, 局部变量起始)
    - EAX: 保存返回结果
  - 64位栈结构: 结构类似
    - rsp: 栈顶指针
    - rbp: 栈基指针
    - rax: 保存返回结果
    - 参数传递时, 前 6个放寄存器中(再由被调用函数 push 进自己的栈, 用以寻址), 参数超过 6个压入栈中

  ### 内核栈

- 内核栈结构

  - Linux 为每个 task 分配了内核栈, 32位(8K), 64位(16K)
  - 栈结构: [预留8字节 +] pt_regs + 内核栈 + 头部 thread_info
  - thread_info 是 task_struct 的补充, 存储于体系结构有关的内容
  - pt_regs 用以保存用户运行上下文, 通过 push 寄存器到栈中保存

  ```
  #define task_pt_regs(task) \
  ({									\
  	unsigned long __ptr = (unsigned long)task_stack_page(task);	\
  	__ptr += THREAD_SIZE - TOP_OF_KERNEL_STACK_PADDING;		\
  	((struct pt_regs *)__ptr) - 1;					\
  })
  
  ```

- 通过 task_struct 找到内核栈

  ```
  static inline void *task_stack_page(const struct task_struct *task)
  {
  	return task->stack;
  }
  
  ```

  - 直接由 task_struct 内的 stack 直接得到指向 thread_info 的指针

- 通过内核栈找到 task_struct

  - 32位 直接由 thread_info 中的指针得到
  - 64位 每个 CPU 当前运行进程的 task_struct 的指针存放到 Per CPU 变量 current_task 中; 可调用 this_cpu_read_stable 进行读取

### 调度策略与调度类

进程包括两类: 实时进程(优先级高); 普通进程。 两种进程调度策略不同: task_struct->policy 指明采用哪种调度策略(有6种策略)

```
unsigned int policy;

#define SCHED_NORMAL		0
#define SCHED_FIFO		    1
#define SCHED_RR		    2
#define SCHED_BATCH		    3
#define SCHED_IDLE		    5
#define SCHED_DEADLINE		6

int prio, static_prio, normal_prio;
unsigned int rt_priority;

```

优先级配合调度策略, 实时进程(0-99); 普通进程(100-139)

#### 实时调度策略

高优先级可抢占低优先级进程

- SCHED_FIFO: 相同优先级进程先来先得
- SCHED_RR: 轮流调度策略, 采用时间片轮流调度相同优先级进程
- SCHED_DEADLINE: 在调度时, 选择 deadline 最近的进程

#### 普通调度策略

- SCHED_NORMAL: 普通进程
- SCHED_BATCH: 后台进程, 可以降低优先级
- SCHED_IDLE: 空闲时才运行

#### 调度策略的调度类

task_struct 中 * sched_class 指向封装了调度策略执行逻辑的类(有5种)

- stop_sched_class: 优先级最高. 将中断其他所有进程, 且不能被打断
- dl_sched_class: 实现 deadline 调度策略
- rt_sched_class: RR 或 FIFO, 具体策略由 task_struct->policy 指定
- fair_sched_class: 普通进程调度
- idle_sched_class: 空闲进程调度

#### 普通进程的 fair 完全公平调度算法 CFS

- 记录进程运行时间( vruntime 虚拟运行时间)
- 优先调度 vruntime 小的进程
- 按照比例累计 vruntime, 使之考虑进优先级关系

```
虚拟运行时间 vruntime += 实际运行时间 delta_exec * NICE_0_LOAD/ 权重
```

#### 调度队列和调度实体

- CFS 中需要对 vruntime 排序找最小, 不断查询更新, 因此利用红黑树实现调度队列
- task_struct 中有 实时调度实体sched_rt_entity, Deadline 调度实体sched_dl_entity 和 完全公平调度实体 sched_entity, cfs 调度实体即红黑树节点
- 所有可运行的进程通过不断地插入操作最终都存储在以时间为顺序的红黑树中，vruntime 最小的在树的左侧，vruntime 最多的在树的右侧。 CFS 调度策略会选择红黑树最左边的叶子节点作为下一个将获得 cpu 的任务
- 每个 CPU 都有 rq 结构体, 里面有 rt_rq 和 cfs_rq 调度队列以及其他信息; 队列描述该 CPU 所运行的所有进程
- 先在 rt_rq 中找进程运行, 若没有再到 cfs_rq 中找; cfs_rq 中 rb_root 指向红黑树根节点, rb_leftmost指向最左节点

#### 调度类如何工作

```
struct sched_class {
	const struct sched_class *next;
...
	void (*enqueue_task) (struct rq *rq, struct task_struct *p, int flags);
	void (*dequeue_task) (struct rq *rq, struct task_struct *p, int flags);
...
	struct task_struct * (*pick_next_task) (struct rq *rq,
						struct task_struct *prev,
						struct rq_flags *rf);

}
extern const struct sched_class stop_sched_class;
extern const struct sched_class dl_sched_class;
extern const struct sched_class rt_sched_class;
extern const struct sched_class fair_sched_class;
extern const struct sched_class idle_sched_class;
const struct sched_class fair_sched_class = {
	.next			= &idle_sched_class,
	...
}
```

- 调度类中有一个成员指向下一个调度类(按优先级顺序串起来)
- 找下一个运行任务时, 按 stop-dl-rt-fair-idle 依次调用调度类, 不同调度类操作不同调度队列
- 对于同样的 pick_next_task 选取下一个要运行的任务这个动作，不同的调度类有自己的实现。fair_sched_class 的实现是 pick_next_task_fair，rt_sched_class 的实现是 pick_next_task_rt。我们会发现这两个函数是操作不同的队列，pick_next_task_rt 操作的是 rt_rq，pick_next_task_fair 操作的是 cfs_rq
- 在每个 CPU 上都有一个队列 rq，这个队列里面包含多个子队列，例如 rt_rq 和 cfs_rq，不同的队列有不同的实现方式。某个 CPU 需要找下一个任务执行的时候，会按照优先级依次调度类，不同的调度类操作不同的队列。当然 rt_sched_class 先被调用，它会在 rt_rq 上找下一个任务，只有找不到的时候，才轮到 fair_sched_class 被调用，它会在 cfs_rq 上找下一个任务。这样保证了实时任务的优先级永远大于普通任务

### 主动调度

调度, 切换运行进程, 有两种方式

- 进程调用 sleep 或等待 I/O, 主动让出 CPU
- 进程运行一段时间, 被动让出 CPU

这个片段可以看作写入块设备的一个典型场景，需要主动让出 CPU

```
static void btrfs_wait_for_no_snapshoting_writes(struct btrfs_root *root)
{
......
	do {
		prepare_to_wait(&root->subv_writers->wait, &wait,
				TASK_UNINTERRUPTIBLE);
		writers = percpu_counter_sum(&root->subv_writers->counter);
		if (writers)
			schedule();
		finish_wait(&root->subv_writers->wait, &wait);
	} while (writers);
}
```

主动让出 CPU 的方式, 调用 schedule(), schedule() 调用 __schedule()

- __schedule() 取出 rq; 取出当前运行进程的 task_struct

- 调用 pick_next_task 取下一个进程

  - 依次调用调度类(优化: 大部分都是普通进程), 因此大多数情况调用 fair_sched_class.pick_next_task[_fair]
  - pick_next_task_fair 先取出 cfs_rq 队列, 取出当前运行进程调度实体, 更新 vruntime
  - pick_next_entity 取最左节点, 并得到 task_struct, 若与当前进程不一样, 则更新红黑树 cfs_rq
  - 选出的继任者和前任不同，就要进行上下文切换，继任者进程正式进入运行

- 进程上下文切换:

  切换进程内存空间（就是虚拟内存）, 切换寄存器和 CPU 上下文(运行 context_switch)

  - context_switch() -> switch_to() -> __switch_to_asm(切换[内核]栈顶指针) -> __switch_to()
  - __switch_to() 取出 Per CPU 的 TTS(Task State Segment，任务状态段)
  - x86 提供以硬件方式切换进程的模式, 为每个进程在内存中维护一个 TTS, TTS 有所有寄存器, 同时 TR(Task Register, 任务寄存器)指向某个 TTS,将会触发硬件保存 CPU 所有寄存器的值到当前进程的 TSS,然后从新进程的 TSS 中读出所有寄存器值，加载到 CPU对应的寄存器中， 更改 TR 会触发换出 TTS(旧进程)和换入 TTS(新进程), 但切换进程没必要换所有寄存器
  - Linux 中每个 CPU 关联一个 TTS, 同时 TR 不变, Linux 中参与进程切换主要是栈顶寄存器
  - 进程切换，就是将某个进程的 thread_struct 里面寄存器的值，写入到 CPU 的 TR 指向的 tss_str_struct，对于 CPU 来讲，这就算是完成了切换
  - task_struct 的 thread 结构体保留切换时需要修改的寄存器, 切换时将新进程 thread 写入 CPU TTS 中
  - 具体各类指针保存位置和时刻
    - 用户栈: 切换进程内存空间时切换
    - 用户栈顶指针: 内核返回用户态时从内核栈 pt_regs 中弹出
    - 用户指令指针: 内核返回用户态时从内核栈 pt_regs 中弹出
    - 内核栈: 由切换的 task_struct 中的 stack 指针指向
    - 内核栈顶指针: __switch_to_asm 修改 sp 后加载到 TTS
    - 内核指令指针: ((last) = __switch_to_asm((pre), (next)))

### 抢占式调度

两种情况: 执行太久, 需切换到另一进程; 另一个高优先级进程被唤醒

- 执行太久: 由时钟中断触发检测, 中断处理调用 scheduler_tick 
  - 取当前进程 task_struct->task_tick_fair()->取 sched_entity cfs_rq 调用 entity_tick()
  - entity_tick() 调用 update_curr 更新当前进程 vruntime, 调用 check_preempt_tick 检测是否需要被抢占
  - check_preempt_tick 中计算 ideal_runtime(一个调度周期中应该运行的实际时间), 若进程本次调度运行时间 > ideal_runtime, 则应该被抢占
  - 还要通过 __pick_first_entiry 取出红黑树中最小的进程，如果当前进程的 vruntime 大于该进程的 vruntime，且差值大于 ideal_runtime, 则应该被抢占 
  - 要被抢占, 则调用 resched_curr, 设置 TIF_NEED_RESCHED, 将其标记为应被抢占进程(因为要等待当前进程运行 `__schedule`)
- 另一个高优先级进程被唤醒: 当 I/O 完成, 进程被唤醒, 若优先级高于当前进程则触发抢占
  - try_to_wake_up()->ttwu_queue() 将唤醒任务加入队列 调用 ttwu_do_activate 激活任务
  - 调用 ttwu_do_wakeup()->check_preempt_curr() 检查是否应该抢占, 若需抢占则标记

#### 抢占时机

让进程调用 `__schedule`, 分为用户态和内核态

- 用户态进程

  - 时机-1: 从系统调用中返回, 返回过程中会调用 exit_to_usermode_loop, 检查 `_TIF_NEED_RESCHED`, 若打了标记, 则调用 schedule()

    ```
    static void exit_to_usermode_loop(struct pt_regs *regs, u32 cached_flags)
    {
    	while (true) {
    		/* We have work to do. */
    		local_irq_enable();
    
    		if (cached_flags & _TIF_NEED_RESCHED)
    			schedule();
    ......
    ```

  - 时机-2: 从中断中返回, 中断返回分为返回用户态和内核态(汇编代码: arch/x86/entry/entry_64.S), 返回用户态过程中会调用 exit_to_usermode_loop()->shcedule()

    ```
    common_interrupt:
    ...
    /* Interrupt came from user space */
    GLOBAL(retint_user)
            mov     %rsp,%rdi
            call    prepare_exit_to_usermode	// 返回用户态
    ...
    /* Returning to kernel space */
    retint_kernel:
    #ifdef CONFIG_PREEMPT
    ...
            call    preempt_schedule_irq		// 返回内核态
            jmp     0b
    ```

- 内核态进程

  - 时机-1: 发生在 preempt_enable() 中, 内核态进程有的操作不能被中断, 会调用 preempt_disable(), 在开启时(调用 preempt_enable) 时是一个抢占时机, 会调用 preempt_count_dec_and_test(), 检测 preempt_count 和标记, 若可抢占则最终调用 `__schedule`
  - 时机-2: 在内核态也会遇到中断的情况，当中断返回的时候，返回的仍然是内核态，也会调用 `__schedule`

```
asmlinkage __visible void __sched preempt_schedule_irq(void)
{
......
	do {
		preempt_disable();
		local_irq_enable();
		__schedule(true);
...
```

### 进程创建

fork是一个系统调用，最后会在 sys_call_table 找到相应的系统调用 sys_fork()->`_do_fork`

创建进程做两件事: 复制初始化 task_struct; 唤醒新进程

```
long _do_fork(...)
{
...
	p = copy_process(clone_flags, stack_start, stack_size,
			 child_tidptr, NULL, trace, tls, NUMA_NO_NODE);
...
		wake_up_new_task(p);
...
```

- 复制并初始化 task_struct, copy_process()

  - dup_task_struct: 分配 task_struct 结构体; 创建内核栈, 赋给`* stack`; 复制 task_struct, 设置 thread_info;
  - copy_creds: 权限相关：分配 cred 结构体并复制, p->cred = p->real_cred = get_cred(new)
  - 初始化运行时统计量
  - sched_fork 调度相关：分配并初始化 sched_entity; state = TASK_NEW; 实际和虚拟运行时间设置为0；设置优先级和调度类，普通进程就是fair_sched_class; 设置调度函数，对于CFS就是 task_fork_fair()，在update_curr 更新当前进程运行统计量, 将当前进程 vruntime 赋给子进程, 通过 sysctl_sched_child_runs_first 设置是否让子进程抢占, 若是则将其 sched_entity 放前头, 并调用 resched_curr 做被抢占标记TIF_NEED_RESCHED
  - 初始化文件和文件系统变量 
    - copy_files: 复制进程打开的文件信息, 用 files_struct 维护; 
    - copy_fs: 复制进程目录信息，包括根目录/根文件系统; pwd 等, 用 fs_struct 维护。每个进程有自己的根目录和根文件系统 root，也有当前目录 pwd 和当前目录的文件系统
  - 初始化信号相关内容: 复制信号和处理函数
  - 复制内存空间: 分配并复制 mm_struct; 复制内存映射信息
  - 分配 pid，设置 tid，group_leader，并且建立进程之间的亲缘关系

- 唤醒新进程 wake_up_new_task()

  ```
  void wake_up_new_task(struct task_struct *p)
  {
  	struct rq_flags rf;
  	struct rq *rq;
  ...
  	p->state = TASK_RUNNING;
  ...
  	activate_task(rq, p, ENQUEUE_NOCLOCK);
  	p->on_rq = TASK_ON_RQ_QUEUED;
  	trace_sched_wakeup_new(p);
  	check_preempt_curr(rq, p, WF_FORK);
  ...
  ```

  - state = TASK_RUNNING; activate 用调度类将当前子进程入队列

  - 如果是 CFS 的调度类，则执行相应的 enqueue_task_fair，取出的队列ifs_rq，再调用enqueue_entiry ，其中会调用 update_curr 更新运行统计量, 再将 sched_entity 加入到红黑树里面

    ```
    static void check_preempt_wakeup(struct rq *rq, struct task_struct *p, int wake_flags)
    {
    	struct task_struct *curr = rq->curr;
    	struct sched_entity *se = &curr->se, *pse = &p->se;
    	struct cfs_rq *cfs_rq = task_cfs_rq(curr);
    ...
    	if (test_tsk_need_resched(curr))
    		return;
    ...
    	find_matching_se(&se, &pse);
    	update_curr(cfs_rq_of(se));
    	if (wakeup_preempt_entity(se, pse) == 1) {
    		goto preempt;
    	}
    	return;
    preempt:
    	resched_curr(rq);
    ...
    ```

  - 调用 check_preempt_curr 看是否能抢占, 若 task_fork_fair 中已设置 sysctl_sched_child_runs_first, 直接返回, 否则还是会调用 update_curr 更新一次统计量，然后 将父进程和子进程 PK 一次，看是不是要抢占，如果要调用 resched_curr 标记父进程为 TIF_NEED_RESCHED

  - 若父进程被标记会被抢占, 则系统调用 fork 返回到用户态的时候会调度子进程

### 线程的创建

线程是由内核态和用户态合作完成的，pthread_create 是 Glibc 库的一个函数，而非系统调用，pthread_create 中，会做以下事情：

1. 设置线程属性参数，如线程栈大小，没有传入，就取默认值

2. 创建用户态维护线程的结构即 pthread

3. 创建线程栈 allocate_stack

   - 如果在线程属性设置过栈的大小，取栈的大小，为防止访问越界，在栈末尾加 guardsize
   - 在进程堆中创建线程栈(先尝试从缓存中取用)
   - 若无缓存线程栈, 调用 `__mmap` 创建
   - 将 pthread 指向栈空间中
   - 计算 guard 内存位置, 并设置保护
   - 填充 pthread 内容, 其中 specific 存放属于线程的全局变量
   - 线程栈放入 stack_used 链表中，表示正在使用(另外 stack_cache 链表记录缓存的线程栈)

4. 设置运行函数start_routine，start_routine 的参数，以及调度策略到 pthread 中

5. 调用 create_thread 创建线程

   如果在进程的主线程里面调用其他系统调用，当前用户态的栈是指向整个进程的栈，栈顶指针也是指向进程的栈，指令指针也是指向进程的主线程的代码，如果不是主线程，这些都会变。将线程要执行的函数的参数和指令的位置都压到栈里面的工作需要自己做：

   - 设置 clone_flags 标志位, 调用 `__clone`
   - clone 系统调用返回时, 应该要返回到新线程上下文中, 因此 `__clone` 将参数和指令位置压入栈中, 返回时从该函数开始执行

6. 内核调用 `__do_fork` 

   - 在 copy_process 复制 task_struct 过程中, 五大数据结构不复制, 直接引用进程的，引用数要加一

   - 亲缘关系设置: group_leader 和 tgid 是当前进程; real_parent 与当前进程一样

   - 信号处理: 数据结构共享, 处理一样

     - copy_process的时候，会初始化p->pending，每个task_struct都有这样一个成员变量，这是一个信号列表，如果这个task_struct是一个线程，则里面的信号是发给这个线程的，如果task_struct是一个进程，则里面的信号是发给主线程的

       ```
       init_sigpending(&p->pending);
       ```

     - 在创建进程的过程中，会初始化 signal_struct 里面的 struct sigpending shared_pending，创建线程会共享，即整个进程的所有线程会共享一个shared_pending，这也是一个信号列表，发给整个进程，哪个线程处理都一样

       ```
       init_sigpending(&sig->shared_pending);
       ```

7. 返回用户态，不是直接返回，而是先运行 start_thread 通用函数

   - 在 start_thread 中调用用户的函数, 运行完释放线程相关数据
   - 如果是最后一个线程直接退出进程
   - 调用 `__free_tcb` 释放 pthread 以及线程栈, 从 stack_used 移到 stack_cache 中

### 内存管理

操作系统会给进程分配一个虚拟地址。所有进程看到的这个地址都是一样的，都是从 0 开始编号。在程序里面，指令写入的地址是虚拟地址。当程序要访问虚拟地址的时候，由内核的数据结构进行转换，转换成不同的物理地址，这样不同的进程运行的时候，写入的是不同的物理地址

内存管理包含：

- 物理内存地址管理
- 虚拟内存地址管理
- 物理内存地址和虚拟内存地址的映射

除了内存管理模块（包括虚拟地址和物理地址映射）, 其他都使用虚拟地址(包括内核)

- 虚拟内存空间包含: 内核空间(高地址); 用户空间(低地址)
- 用户空间从低到高布局为: Text Segment（二进制程序的代码段）、Data Segment（静态常量） 和 BSS Segment（未初始化静态变量）、堆段（malloc分配的空间）、Memory Mapping Segment（内存映射段，如so库文件）、 栈地址空间段（主线程的函数调用的函数栈存放在这）；多个进程的用户空间是独立的
- 多个进程的内核空间是共享的，但内核栈每个进程不一样；内核代码也仅能访问内核空间；内核也有内核代码段, DATA 段, 和 BSS 段，位于内核空间低地址；内核代码也是 ELF 格式

查看进程内存空间分布情况：

```
cat /proc/$pid/map		// mac下不行
```

