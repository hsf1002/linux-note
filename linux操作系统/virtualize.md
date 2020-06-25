### 虚拟化

##### 三种虚拟化方式

* 完全虚拟化（Full virtualization）：虚拟化软件会模拟假的 CPU、内存、网络、硬盘，其实再向内核申请资源
* 硬件辅助虚拟化（Hardware-Assisted Virtualization）：不需要虚拟化软件在中间转述，除非遇到特别敏感的指令，才需要将标志位设为物理机内核态运行，这样大大提高了效率
* 半虚拟化（Paravirtualization）：为了取得更高的性能，访问网络或者硬盘的时候，需要让虚拟机内核加载特殊的驱动，不能像访问物理机一样访问网络或者硬盘，而是用一种特殊的方式

桌面虚拟化软件多采用 VirtualBox，服务器的虚拟化软件多使用 qemu，其中关键字 emu，全称是 emulator。qemu 向 Guest OS 模拟 CPU，也模拟其他的硬件，GuestOS 认为自己和硬件直接打交道，其实是同 qemu 模拟出来的硬件打交道，qemu 会将这些指令转译给真正的硬件。由于所有的指令都要从 qemu 里面过一手，因而性能就会比较差。所以要使用硬件辅助虚拟化技术 Intel-VT，AMD-V，需要 CPU 硬件开启这个标志位，一般在 BIOS 里面设置。开启标志位之后，通过 KVM，GuestOS 的 CPU 指令不用经过 Qemu 转译，直接运行，大大提高了速度。KVM 在内核里面需要有一个模块，来设置当前 CPU 是 Guest OS 在用，还是 Host OS 在用。

查看内核模块中是否含有 kvm： `lsmod | grep kvm`

![img](https://static001.geekbang.org/resource/image/f5/62/f5ee1a44d7c4890e411c2520507ddc62.png)

qemu 和 kvm 整合之后，CPU 的性能问题解决了。Qemu 还会模拟其他的硬件，如网络和硬盘。qemu 采取半虚拟化的方式，让 Guest OS 加载特殊的驱动来做这件事情。例如，网络需要加载 virtio_net，存储需要加载 virtio_blk，Guest 需要安装这些半虚拟化驱动，GuestOS 知道自己是虚拟机，所以数据会直接发送给半虚拟化设备，经过特殊处理（例如排队、缓存、批量处理等性能优化方式），最终发送给真正的硬件。在一定程度上提高了性能

![img](https://static001.geekbang.org/resource/image/f7/22/f748fd6b6b84fa90a1044a92443c3522.png)

##### 创建虚拟机

可以使用 qemu-kvm 或 VirtualBox 创建过虚拟机，如果在桌面虚拟化软件上选择桥接网络，在笔记本电脑上，就会形成下面的结构

![img](https://static001.geekbang.org/resource/image/2b/47/2b49867c473162d4706553e8cbb5f247.png)

每个虚拟机都会有虚拟网卡，在笔记本电脑上，会发现多了几个网卡，其实是虚拟交换机。这个虚拟交换机将虚拟机连接在一起。如果使用桥接网络，虚拟机的地址和笔记本电脑的地址，以及旁边同事的电脑的网段是一个网段。相当于这个网桥上有三台机器，是一个网段

![img](https://static001.geekbang.org/resource/image/78/47/7899a96aaa0b91c165f867d3ec42e947.png)

在数据中心里面，采取类似的技术，只不过是 Linux 在每台机器上都创建网桥 br0，虚拟机的网卡都连到 br0 上，物理网卡也连到 br0 上，所有的 br0 都通过物理网卡连接到物理交换机上

![img](https://static001.geekbang.org/resource/image/da/a7/da83bb01b7ed63ac0062b5cc835099a7.png)

虚拟机会和物理网络具有相同的网段，就相当于两个虚拟交换机、一个物理交换机，一共三个交换机连在一起。两组四个虚拟机和两台物理机都是在一个二层网络里面的

![img](https://static001.geekbang.org/resource/image/8e/c6/8e471a287e0181f1b7af56b60b84adc6.png)

* 虚拟化的本质是用 qemu 的软件模拟硬件，但是模拟方式比较慢，需要加速
* 虚拟化主要模拟 CPU、内存、网络、存储，分别有不同的加速办法
* CPU 和内存主要使用硬件辅助虚拟化进行加速，需要配备特殊的硬件才能工作
* 网络和存储主要使用特殊的半虚拟化驱动加速，需要加载特殊的驱动程序

### 虚拟化之CPU

定义一个 qemu 模块会调用 type_init。例如kvm 的模块在 accel/kvm/kvm-all.c 文件

![img](https://static001.geekbang.org/resource/image/07/30/078dc698ef1b3df93ee9569e55ea2f30.png)

每个模块都会有一个定义 TypeInfo，通过 type_init 变为全局的 TypeImpl。TypeImpl 有以下成员：

* name 表示当前类型的名称
* parent 表示父类的名称
* class_init 用于将 TypeImpl 初始化为 MachineClass
* instance_init 用于将 MachineClass 初始化为 MachineState

CPU 的虚拟化过程：

1. 首先，定义 CPU 类型的 TypeInfo 和 TypeImpl、继承关系，并且声明它的类初始化函数
2. 在 qemu 的 main 函数中调用 MachineClass 的 init 函数，既会初始化 CPU，也会初始化内存
3. CPU 初始化的时候，会调用 pc_new_cpu 创建一个虚拟 CPU，它会调用 CPU 这个类的初始化函数
4. 每一个虚拟 CPU 会调用 qemu_thread_create 创建一个线程，线程的执行函数为 qemu_kvm_cpu_thread_fn
5. 在虚拟 CPU 对应的线程执行函数中，先调用 kvm_vm_ioctl(KVM_CREATE_VCPU)，在内核的 KVM 里面，创建一个结构 struct vcpu_vmx，表示这个虚拟 CPU。在这个结构里面，有一个 VMCS，用于保存当前虚拟机 CPU 的运行时的状态，用于状态切换
6. 在虚拟 CPU 对应的线程执行函数中，继续调用 kvm_vcpu_ioctl(KVM_RUN)，在内核的 KVM 里面，运行这个虚拟机 CPU。运行方式是保存宿主机的寄存器，加载客户机的寄存器，然后调用 `__ex(ASM_VMX_VMLAUNCH)` 或者 `__ex(ASM_VMX_VMRESUME)`，进入客户机模式运行。一旦退出客户机模式，就会保存客户机寄存器，加载宿主机寄存器，进入宿主机模式运行，并且会记录退出虚拟机模式的原因。大部分的原因是等待 I/O，因而宿主机调用 kvm_handle_io 进行处理

![img](https://static001.geekbang.org/resource/image/c4/67/c43639f7024848aa3e828bcfc10ca467.png)

##### 虚拟化之内存

CPU 的虚拟化是用户态的 qemu 和内核态的 KVM 共同配合完成的。二者通过 ioctl 进行通信。对于内存管理来讲，也需要这两者配合完成。对于内存管理，操作系统给每个进程分配的内存都是虚拟内存，需要通过页表映射，变成物理内存进行访问。当有了虚拟机之后，分为四类：

* 虚拟机里面的虚拟内存（Guest OS Virtual Memory，GVA），虚拟机里面的进程看到的内存空间
* 虚拟机里面的物理内存（Guest OS Physical Memory，GPA），虚拟机里面的操作系统看到的内存，它认为这是物理内存
* 物理机的虚拟内存（Host Virtual Memory，HVA），物理机上的 qemu 进程看到的内存空间
* 物理机的物理内存（Host Physical Memory，HPA），物理机上的操作系统看到的内存

 CPU 和内存紧密结合，内存虚拟化的初始化过程，和 CPU 虚拟化的初始化是一起完成的，而内存映射对于虚拟机来讲非常麻烦，从 GVA 到 GPA 到 HVA 到 HPA，性能很差，为了解决这个问题，有两种主要的思路：

* 影子页表：内存映射要通过页表来管理，页表地址应该放在 cr3 寄存器里面。本来的过程是，客户机要通过 cr3 找到客户机的页表，实现从 GVA 到 GPA 的转换，然后在宿主机上，要通过 cr3 找到宿主机的页表，实现从 HVA 到 HPA 的转换。为了实现客户机虚拟地址空间到宿主机物理地址空间的直接映射。客户机中每个进程都有自己的虚拟地址空间，所以 KVM 需要为客户机中的每个进程页表都要维护一套相应的影子页表。在客户机访问内存时，使用的不是客户机的原来的页表，而是这个页表对应的影子页表，从而实现了从客户机虚拟地址到宿主机物理地址的直接转换。在 TLB 和 CPU 缓存上缓存的是来自影子页表中客户机虚拟地址和宿主机物理地址之间的映射，因此提高了缓存的效率。但是影子页表的引入也意味着 KVM 需要为每个客户机的每个进程的页表都要维护一套相应的影子页表，内存占用比较大，且客户机页表和和影子页表也需要进行实时同步
* 扩展页表：硬件的方式，Intel 的 EPT（Extent Page Table）技术。EPT 在原有客户机页表对客户机虚拟地址到客户机物理地址映射的基础上，又引入了 EPT 页表来实现客户机物理地址到宿主机物理地址的另一次映射。客户机运行时，客户机页表被载入 CR3，而 EPT 页表被载入专门的 EPT 页表指针寄存器 EPTP。有了 EPT，在客户机物理地址到宿主机物理地址转换的过程中，缺页会产生 EPT 缺页异常。KVM 首先根据引起异常的客户机物理地址，映射到对应的宿主机虚拟地址，然后为此虚拟地址分配新的物理页，最后 KVM 再更新 EPT 页表，建立起引起异常的客户机物理地址到宿主机物理地址之间的映射。KVM 只需为每个客户机维护一套 EPT 页表，也大大减少了内存的开销。 EPT 重点解决的就是从 GPA 到 HPA 的转换问题。因为要经过两次页表，所以 EPT 又称为 tdp（two dimentional paging）。EPT 的页表结构也是分为四层，EPT Pointer （EPTP）指向 PML4 的首地址

![img](https://static001.geekbang.org/resource/image/02/30/02e4740398bc3685f366351260ae7230.jpg)

虚拟机的内存管理需要用户态的 qemu 和内核态的 KVM 共同完成。为了加速内存映射，需要借助硬件的 EPT 技术

1. 在用户态 qemu 中，有一个结构 AddressSpace address_space_memory 来表示虚拟机的系统内存，它可能包含多个内存区域 struct MemoryRegion，组成树形结构，指向由 mmap 分配的虚拟内存
2. 在 AddressSpace 结构中，有一个 struct KVMMemoryListener，当有新的内存区域添加的时候，会被通知调用 kvm_region_add 来通知内核
3. 在用户态 qemu 中，对于虚拟机有一个结构 struct KVMState 表示这个虚拟机，它会指向一个数组的 struct KVMSlot 表示该虚拟机的多个内存条，KVMSlot 中有一个 void *ram 指针指向 mmap 分配的那块虚拟内存
4. kvm_region_add 是通过 ioctl 来通知内核 KVM 的，会给内核 KVM 发送一个 KVM_SET_USER_MEMORY_REGION 消息，表示用户态 qemu 添加了一个内存区域，内核 KVM 也应该添加一个相应的内存区域
5. 和用户态 qemu 对应的内核 KVM，对于虚拟机有一个结构 struct kvm 表示这个虚拟机，这个结构会指向一个数组的 struct kvm_memory_slot 表示这个虚拟机的多个内存条，kvm_memory_slot 中有起始页号，页面数目，表示这个虚拟机的物理内存空间
6. 虚拟机的物理内存空间里面的页面当然不是一开始就映射到物理页面的，只有当虚拟机的内存被访问的时候，即 mmap 分配的虚拟内存空间被访问的时候，先查看 EPT 页表，是否已经映射过，如果已经映射过，则经过四级页表映射，就能访问到物理页面
7. 如果没有映射过，则虚拟机会通过 VM-Exit 指令回到宿主机模式，通过 handle_ept_violation 补充页表映射。先是通过 handle_mm_fault 为虚拟机的物理内存空间分配真正的物理页面，然后通过 __direct_map 添加 EPT 页表映射

![img](https://static001.geekbang.org/resource/image/01/9b/0186c533b7ef706df880dfd775c2449b.jpg)

##### 虚拟化之存储

在虚拟化技术的早期，不同的虚拟化技术会针对不同硬盘设备和网络设备实现不同的驱动，虚拟机里面的操作系统也要根据不同的虚拟化技术和物理存储和网络设备，选择加载不同的驱动。由于硬盘设备和网络设备太多，驱动纷繁复杂。后来慢慢就形成了一定的标准：virtio。virtio 负责对于虚拟机提供统一的接口。在虚拟机里面的操作系统加载的驱动，以后都统一加载 virtio 就可以了

![img](https://static001.geekbang.org/resource/image/1e/33/1e13ffd5ac846c52739291cb489d0233.png)

virtio 的架构可以分为四层：

* 首先，在虚拟机里面的 virtio 前端，针对不同类型的设备有不同的驱动程序，但是接口都是统一的。例如，硬盘就是 virtio_blk，网络就是 virtio_net
* 其次，在宿主机的 qemu 里面，实现 virtio 后端的逻辑，主要就是操作硬件的设备。例如通过写一个物理机硬盘上的文件来完成虚拟机写入硬盘的操作。再如向内核协议栈发送一个网络包完成虚拟机对于网络的操作
* 在 virtio 的前端和后端之间，有一个通信层，里面包含 virtio 层和 virtio-ring 层。virtio 这一层实现的是虚拟队列接口，是前后端通信的桥梁。而 virtio-ring 则是该桥梁的具体实现
* virtio 使用 virtqueue 进行前端和后端的高速通信。不同类型的设备队列数目不同。virtio-net 使用两个队列，一个用于接收，另一个用于发送；而 virtio-blk 仅使用一个队列。如果客户机要向宿主机发送数据，客户机会将数据的 buffer 添加到 virtqueue 中，然后通过写入寄存器通知宿主机。这样宿主机就可以从 virtqueue 中收到的 buffer 里面的数据

![img](https://static001.geekbang.org/resource/image/2e/f3/2e9ef612f7b80ec9fcd91e200f4946f3.png)

VirtIODevice，VirtQueue，vring 之间的关系如下图所示：

![img](https://static001.geekbang.org/resource/image/e1/6d/e18dae0a5951392c4a8e8630e53a616d.jpg)

存储虚拟化的过程分为前端、后端和中间的队列：

* 前端有前端的块设备驱动 Front-end driver，在客户机的内核里面，它符合普通设备驱动的格式，对外通过 VFS 暴露文件系统接口给客户机里面的应用
* 后端有后端的设备驱动 Back-end driver，在宿主机的 qemu 进程中，当收到客户机的写入请求的时候，调用文件系统的 write 函数，写入宿主机的 VFS 文件系统，最终写到物理硬盘设备上的 qcow2 文件
* 中间的队列用于前端和后端之间传输数据，在前端的设备驱动和后端的设备驱动，都有类似的数据结构 virt-queue 来管理这些队列

![img](https://static001.geekbang.org/resource/image/1f/4b/1f0c3043a11d6ea1a802f7d0f3b0b34b.jpg)

qemu 初始化的时候，virtio 的后端有数据结构 VirtIODevice，VirtQueue 和 vring 一模一样，前端和后端对应起来，都应该指向刚才创建的那一段内存。qemu 后端的 VirtIODevice 的 VirtQueue 的 vring 的地址，被设置成了刚才给队列分配的内存的 GPA

![img](https://static001.geekbang.org/resource/image/25/d0/2572f8b1e75b9eaab6560866fcb31fd0.jpg)

中间 virtio 队列的格式：

![img](https://static001.geekbang.org/resource/image/49/db/49414d5acc81933b66410bbba102b0db.jpg)

vring 包含三个成员：

* vring_desc：指向分配的内存块，用于存放客户机和 qemu 之间传输的数据
* avail->ring[]：发送端维护的环形队列，指向需要接收端处理的 vring_desc
* used->ring[]：接收端维护的环形队列，指向自己已经处理过了的 vring_desc

存储虚拟化的场景下整个写入的过程：

1. 在虚拟机里面，应用层调用 write 系统调用写入文件
2. write 系统调用进入虚拟机里面的内核，经过 VFS，通用块设备层，I/O 调度层，到达块设备驱动
3. 虚拟机里面的块设备驱动 virtio_blk 和通用的块设备驱动一样，有一个 request  queue，另外有一个函数 make_request_fn 会被设置为 blk_mq_make_request，这个函数用于将请求放入队列
4. 虚拟机里面的块设备驱动 virtio_blk 会注册一个中断处理函数 vp_interrupt。当 qemu 写入完成之后，它会通知虚拟机里面的块设备驱动
5. blk_mq_make_request 最终调用 virtqueue_add，将请求添加到传输队列 virtqueue 中，然后调用 virtqueue_notify 通知 qemu
6. 在 qemu 中，本来虚拟机正处于 KVM_RUN 的状态，即客户机状态，qemu 收到通知后，通过 VM exit 指令退出客户机状态，进入宿主机状态，根据退出原因，得知有 I/O 需要处理
7. qemu 调用 virtio_blk_handle_output，最终调用 virtio_blk_handle_vq，virtio_blk_handle_vq 里面有一个循环，在循环中，virtio_blk_get_request 函数从传输队列中拿出请求，然后调用 virtio_blk_handle_request 处理请求
8. virtio_blk_handle_request 会调用 blk_aio_pwritev，通过 BlockBackend 驱动写入 qcow2 文件
9. 写入完毕之后，virtio_blk_req_complete 会调用 virtio_notify 通知虚拟机里面的驱动。数据写入完成，刚才注册的中断处理函数 vp_interrupt 会收到这个通知

![img](https://static001.geekbang.org/resource/image/79/0c/79ad143a3149ea36bc80219940d7d00c.jpg)

##### 虚拟化之网络

网络虚拟化有和存储虚拟化类似的地方，它们都是基于 virtio，因而在看网络虚拟化的过程中，有和存储虚拟化很像的数据结构和原理。网络虚拟化也有自己的特殊性。例如，存储虚拟化是将宿主机上的文件作为客户机上的硬盘，而网络虚拟化需要依赖于内核协议栈进行网络包的封装与解封装

qemu 会将客户机发送给它的网络包，转换成为文件流，写入"/dev/net/tun"字符设备。内核中 TUN/TAP 字符设备驱动会收到这个写入的文件流，然后交给 TUN/TAP 的虚拟网卡驱动。它会将文件流再次转成网络包，交给 TCP/IP 栈，最终从虚拟 TAP 网卡 tap0 发出来，成为标准的网络包

![img](https://static001.geekbang.org/resource/image/24/d3/243e93913b18c3ab00be5676bef334d3.png)

内核的实现在 drivers/net/tun.c 文件中。这是一个字符设备驱动程序，里面注册了一个 tun_miscdev 字符设备，从它的定义可以看出，这就是"/dev/net/tun"字符设备

```
static struct miscdevice tun_miscdev = {
  .minor = TUN_MINOR,
  .name = "tun",
  .nodename = "net/tun",
  .fops = &tun_fops,
};

static const struct file_operations tun_fops = {
  .owner  = THIS_MODULE,
  .llseek = no_llseek,
  .read_iter  = tun_chr_read_iter,
  .write_iter = tun_chr_write_iter,
  .poll  = tun_chr_poll,
  .unlocked_ioctl  = tun_chr_ioctl,
  .open  = tun_chr_open,
  .release = tun_chr_close,
  .fasync = tun_chr_fasync,
};
```

在 struct tun_file 中，有一个成员 struct tun_struct，它里面有一个 struct net_device，这个用来表示宿主机上的 tuntap 网络设备。在 struct tun_file 中，还有 struct socket 和 struct sock，因为要用到内核的网络协议栈，所以就需要这两个结构，"/dev/net/tun"对应的 struct file 的 private_data 指向它，因而可以接收 qemu 发过来的数据。除此之外，它还可以通过 struct sock 来操作内核协议栈，然后将网络包从宿主机上的 tuntap 网络设备发出去，宿主机上的 tuntap 网络设备对应的 struct net_device 也归它管，tun_set_iff 创建了 struct tun_struct 和 struct net_device，并且将这个 tuntap 网络设备通过 register_netdevice 注册到内核中。这样就能在宿主机上通过 ip addr 看到这个网卡了

![img](https://static001.geekbang.org/resource/image/98/fd/9826223c7375bec19bd13588f3875ffd.png)

网络虚拟化场景下网络包的发送过程：

1. 在虚拟机里面的用户态，应用程序通过 write 系统调用写入 socket
2. 写入的内容经过 VFS 层，内核协议栈，到达虚拟机里面的内核的网络设备驱动，即 virtio_net
3. virtio_net 网络设备有一个操作结构 struct net_device_ops，里面定义了发送一个网络包调用的函数为 start_xmit
4. 在 virtio_net 的前端驱动和 qemu 中的后端驱动之间，有两个队列 virtqueue，一个用于发送，一个用于接收。需要在 start_xmit 中调用 virtqueue_add，将网络包放入发送队列，然后调用 virtqueue_notify 通知 qemu
5. qemu 本来处于 KVM_RUN 的状态，收到通知后，通过 VM exit 指令退出客户机模式，进入宿主机模式。发送网络包的时候，virtio_net_handle_tx_bh 函数会被调用
6. 接下来是一个 for 循环，在循环中调用 virtqueue_pop，从传输队列中获取要发送的数据，然后调用 qemu_sendv_packet_async 进行发送
7. qemu 会调用 writev 向字符设备文件写入，进入宿主机的内核
8. 在宿主机内核中字符设备文件的 file_operations 里面的 write_iter 会被调用，即 tun_chr_write_iter。在 tun_chr_write_iter 函数中，tun_get_user 将要发送的网络包从 qemu 拷贝到宿主机内核里面来，然后调用 netif_rx_ni 开始调用宿主机内核协议栈进行处理
9. 宿主机内核协议栈处理完毕之后，会发送给 tap 虚拟网卡，完成从虚拟机里面到宿主机的整个发送过程

![img](https://static001.geekbang.org/resource/image/e3/44/e329505cfcd367612f8ae47054ec8e44.jpg)

