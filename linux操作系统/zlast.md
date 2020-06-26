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

