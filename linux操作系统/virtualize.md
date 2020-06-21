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

