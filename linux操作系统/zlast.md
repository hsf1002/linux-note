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