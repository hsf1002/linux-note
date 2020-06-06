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

![img](https://static001.geekbang.org/resource/image/af/83/afa4beefd380effefb0e54a8d9345c83.jpeg)

![img](https://static001.geekbang.org/resource/image/4e/9d/4ed91c744220d8b4298237d2ab2eda9d.jpeg)

- 分段

  ![img](https://static001.geekbang.org/resource/image/7c/04/7c82068d2d6bdb601084a07569ac8b04.jpg)

  - 虚拟地址 = 段选择子(段寄存器) + 段内偏移量
  - 段选择子 = 段号(段表索引) + 标识位
  - 段表 = 物理基地址 + 段界限(偏移量范围) + 特权等级

- Linux 分段实现

  - 段表称为段描述符表, 放在全局标识符表GDT（Global Descriptor Table）中
  - Linux 将段基地址都初始化为 0, 不用于地址映射，Linux 倾向于另外一种从虚拟地址到物理地址的转换方式，称为分页（Paging）
  - Linux 分段功能主要用于权限检查

- Linux 通过分页实现映射

  ![img](https://static001.geekbang.org/resource/image/ab/40/abbcafe962d93fac976aa26b7fcb7440.jpg)

  - 物理内存被换分为大小固定(4KB)的页, 物理页可在内存与硬盘间换出/换入

  - 页表 = 虚拟页号 + 物理页号; 用于定位页

  - 虚拟地址 = 虚拟页号 + 页内偏移

    ![img](https://static001.geekbang.org/resource/image/84/eb/8495dfcbaed235f7500c7e11149b2feb.jpg)

    ![img](https://static001.geekbang.org/resource/image/b6/b8/b6960eb0a7eea008d33f8e0c4facc8b8.jpg)

  - 若采用单页表, 32位系统中一个页表将有 1M 页表项, 占用 4MB(每项 4B)

    ![img](https://static001.geekbang.org/resource/image/42/0b/42eff3e7574ac8ce2501210e25cd2c0b.jpg)

  - Linux 32位系统采用两级页表: 页表目录(1K项, 10bit) + 页表(1K项, 10bit)(页大小(4KB, 12bit))

  - 映射 4GB 内存理论需要 1K 个页表目录项 + 1K\*1K=1M 页表项, 将占用 4KB+4MB 空间

  - 因为完整的页表目录可以满足所有地址的查询, 因此页表只需在对应地址有内存分配时才生成

  - 64 为系统采用 4 级页表

![img](https://static001.geekbang.org/resource/image/7d/91/7dd9039e4ad2f6433aa09c14ede92991.jpg)



### 用户态和内核态的虚拟内存空间

- 内存管理信息在 task_struct 的 mm_struct 中
- task_size 指定用户态虚拟地址大小
  - 32 位系统：3G 用户态, 1G 内核态
  - 64 位系统(只利用 48 bit 地址): 128T 用户态; 128T 内核态

![img](https://static001.geekbang.org/resource/image/89/59/89723dc967b59f6f49419082f6ab7659.jpg)

- 用户态地址空间布局和管理

  ```
  // 表示虚拟地址空间中用于内存映射的起始地址。一般情况下，这个空间是从高地址到低地址增长的。malloc 申请一大块内存的时候，就是通过 mmap 在这里映射一块区域到物理内存。加载动态链接库 so 文件，也是在这个区域里面，映射一块区域到 so 文件
  unsigned long mmap_base;  /* base of mmap area */
  // 总共映射的页的数目
  unsigned long total_vm;    /* Total pages mapped */
  // 被锁定不能换出
  unsigned long locked_vm;  /* Pages that have PG_mlocked set */
  // 不能换出，也不能移动
  unsigned long pinned_vm;  /* Refcount permanently increased */
  // 存放数据的页的数目
  unsigned long data_vm;    /* VM_WRITE & ~VM_SHARED & ~VM_STACK */
  // 存放可执行文件的页的数目
  unsigned long exec_vm;    /* VM_EXEC & ~VM_WRITE & ~VM_STACK */
  // 栈所占的页的数目
  unsigned long stack_vm;    /* VM_STACK */
  // start_code 和 end_code 表示可执行代码的开始和结束位置，start_data 和 end_data 表示已初始化数据的开始位置和结束位置
  unsigned long start_code, end_code, start_data, end_data;
  // start_brk 是堆的起始位置，brk 是堆当前的结束位置，start_stack 是栈的起始位置，栈的结束位置在寄存器的栈顶指针中
  unsigned long start_brk, brk, start_stack;
  // arg_start 和 arg_end 是参数列表的位置， env_start 和 env_end 是环境变量的位置。都位于栈中最高地址的地方
  unsigned long arg_start, arg_end, env_start, env_end;
  ```

  - mm_struct 中有映射页的统计信息(总页数, 锁定页数, 数据/代码/栈映射页数等)以及各区域地址
  - 有 vm_area_struct 描述各个区域(代码/数据/栈等)的属性(包含起始/终止地址, 可做的操作等), 通过链表和红黑树管理
  - 在 load_elf_bianry 时做 vm_area_struct 与各区域的映射, 并将 elf 映射到内存, 将依赖 so 添加到内存映射

  ![img](https://static001.geekbang.org/resource/image/7a/4c/7af58012466c7d006511a7e16143314c.jpeg)

  - 在函数调用时会修改栈顶指针; malloc 分配内存时会修改对应的区域信息(调用 brk 堆; 或调用 mmap 内存映射)
  - brk 判断是否需要分配新页, 并做对应操作; 需要分配新页时需要判断能否与其他 vm_area_struct 合并

- 内核地址空间布局和管理

  - 所有进程看到的内核虚拟地址空间是同一个

  ![img](https://static001.geekbang.org/resource/image/83/04/83a6511faf802014fbc2c02afc397a04.jpg)

  - 32 位系统, 内核态虚拟地址空间一共就 1G, 前 896MB 为直接映射区(虚拟地址 - 3G = 物理地址)

    - 直接映射区也需要建立页表, 通过虚拟地址访问(除了内存管理模块)

    ```
    在内核里面，有两个宏:
    __pa(vaddr) 返回与虚拟地址 vaddr 相关的物理地址
    __va(paddr) 则计算出对应于物理地址 paddr 的虚拟地址
    
    具体的物理内存布局可以查看: /proc/iomem
    ```

    - 直接映射区组成: 1MB 启动时占用; 然后是内核代码/全局变量/BSS等,即 内核 ELF文件内容; 进程 task_struct 即内核栈也在其中
    - 896MB 也称为高端内存(指物理内存)
    - 剩余虚拟空间组成: 8MB 空余; 内核动态映射空间(动态分配内存, 映射放在内核页表中); 持久内存映射(储存物理页信息); 固定内存映射; 临时内存映射(例如为进程映射文件时使用)

  - 64 位系统: 8T 空余; 64T 直接映射区域; 32T(动态映射); 1T(物理页描述结构 struct page); 512MB(内核代码, 也采用直接映射)

![img](https://static001.geekbang.org/resource/image/7e/f6/7eaf620768c62ff53e5ea2b11b4940f6.jpg)

进程运行状态在 32 位下对应关系:

![img](https://static001.geekbang.org/resource/image/28/e8/2861968d1907bc314b82c34c221aace8.jpeg)

进程运行状态在 64 位下对应关系:

![img](https://static001.geekbang.org/resource/image/2a/ce/2ad275ff8fdf6aafced4a7aeea4ca0ce.jpeg)

### 物理内存的组织方式

- 每个物理页由 struct page 表示

  ![img](https://static001.geekbang.org/resource/image/8f/49/8f158f58dda94ec04b26200073e15449.jpeg)

  - 物理页连续, page 放入一个数组中, 称为平坦内存模型
  - 多个 CPU 通过总线访问内存, 称为 SMP 对称多处理器(采用平坦内存模型, 总线成为瓶颈)
  - 每个 CPU 都有本地内存, 访问内存不用总线, 称为 NUMA 非一致内存访问
  - 本地内存称为 NUMA 节点, 本地内存不足可以向其他节点申请
  - NUMA 采用非连续内存模型，页号不连续
  - 另外若内存支持热插拔，则采用稀疏内存模型

- 节点

  - 用 pglist_data 表示 NUMA 节点，多个节点信息保存在 node_data 数组中
  - pglist_data 包括 id，page 数组,起始页号, 总页数, 可用页数
  - 节点分为多个区域 zone, 包括 DMA; 直接映射区; 高端内存区; 可移动区(避免内存碎片)

  ```
  enum zone_type {
  #ifdef CONFIG_ZONE_DMA
  // CPU 只需向 DMA 控制器下达指令，让 DMA 控制器来处理数据的传送，数据传送完毕再把信息反馈给 CPU，这样就可以解放 CPU
    ZONE_DMA,
  #endif
  #ifdef CONFIG_ZONE_DMA32
  // 对于 64 位系统，有两个 DMA 区域。除了上面说的 ZONE_DMA，还有 ZONE_DMA32
    ZONE_DMA32,
  #endif
  // 直接映射区
    ZONE_NORMAL,
  #ifdef CONFIG_HIGHMEM
  // 高端内存区
    ZONE_HIGHMEM,
  #endif
  // 可移动区域，通过将物理内存划分为可移动分配区域和不可移动分配区域来避免内存碎片
    ZONE_MOVABLE,
    __MAX_NR_ZONES
  };
  ```

- 区域 zone

  - 用 zone 表示; 包含第一个页页号; 区域总页数; 区域实际页数; 被伙伴系统管理的页数; 用 per_cpu_pageset 区分冷热页(热页, 被 CPU 缓存的页)
  - 内存分成了节点，把节点分成了区域

- 页

  - 组成物理内存的基本单位, 用 struct page 表示, 有多种使用模式, 因此 page 结构体多由 union 组成
  - 使用一整个页: 1) 直接和虚拟地址映射(匿名页); 2) 与文件关联再与虚拟地址映射(内存映射文件)
    - page 记录: 标记用于内存映射; 指向该页的页表数; 换出页的链表; 复合页, 用于合成大页;
  - 分配小块内存:
    - Linux 采用 slab allocator 技术; 申请一整页, 分为多个小块存储池, 用队列维护其状态(较复杂)
    - slub allocator 更简单
    - slob allocator 用于嵌入式
    - page 记录: 第一个 slab 对象; 空闲列表; 待释放列表

- 页分配

  - 分配较大内存(页级别), 使用伙伴系统
  - Linux 中的内存管理的“页”大小为 4KB。把所有的空闲页分组为 11 个页块链表，每个块链表分别包含很多个大小的页块，有 1、2、4、8、16、32、64、128、256、512 和 1024 个连续页的页块。最大可以申请 1024 个连续页，对应 4MB 大小的连续内存。每个页块的第一个页的物理地址是该页块大小的整数倍

  ![img](https://static001.geekbang.org/resource/image/27/cf/2738c0c98d2ed31cbbe1fdcba01142cf.jpeg)

  - 分配大页剩下的内存, 插入对应空闲链表
  - alloc_pages->alloc_pages_current 用 gfp 指定在哪个 zone 分配

如果有多个 CPU，就有多个节点。每个节点用 struct pglist_data 表示，放在一个数组里面。每个节点分为多个区域，每个区域用 struct zone 表示，也放在一个数组里面。每个区域分为多个页。为了方便分配，空闲页放在 struct free_area 里面，使用伙伴系统进行管理和分配，每一页用 struct page 表示

![img](https://static001.geekbang.org/resource/image/3f/4f/3fa8123990e5ae2c86859f70a8351f4f.jpeg)

### 小内存的分配

- 小内存分配, 例如分配 task_struct 对象

  - 会调用 kmem_cache_alloc_node 函数, 从 task_struct 缓存区域 task_struct_cachep(在系统初始化时, 由 kmem_cache_create 创建) 分配一块内存
  - 使用 task_struct 完毕后, 调用 kmem_cache_free 回收到缓存池中
  - struct kmem_cache 用于表示缓存区信息, 缓存区即分配连续几个页的大块内存, 再切成小内存
  - 小内存即缓存区的每一项, 都由对象和指向下一项空闲小内存的指针组成(随机插入/删除+快速查找空闲)
  - struct kmem_cache 中三个 kmem_cache_order_objects 表示不同的需要分配的内存块大小的阶数和对象数

  ![img](https://static001.geekbang.org/resource/image/45/0a/45f38a0c7bce8c98881bbe8b8b4c190a.jpeg)

  - 分配缓存的小内存块由两个路径 fast path 和 slow path , 分别对应 struct kmem_cache 中的 kmem_cache_cpu 和 kmem_cache_node, 分配时先从 kmem_cache_cpu 分配, 若其无空闲, 再从 kmem_cache_node 分配, 还没有就从伙伴系统申请新内存块
  - struct kmem_cache_cpu 中
    - page 指向大内存块的第一个页, freelist 指向大内存块中第一个空闲项, partial 指向另一个大内存块的第一个页, 但该内存块有部分已分配出去, 当 page 满后, 在 partial 中找
  - struct kmem_cache_node
    - 也有 partial, 是一个链表, 存放部分空闲的多个大内存块, 若 kmem_cacche_cpu 中的 partial 也无空闲, 则在这找
  - 分配过程
    - kmem_cache_alloc_node->slab_alloc_node
    - 快速通道, 取出 kmem_cache_cpu 的 freelist , 若有空闲直接返回
    - 普通通道, 若 freelist 无空闲, 调用 `__slab_alloc`
    - `__slab_alloc` 会重新查看 freelist, 若还不满足, 查看 kmem_cache_cpu 的 partial
    - 若 partial 不为空, 用其替换 page, 并重新检查是否有空闲
    - 若还是无空闲, 调用 new_slab_objects
    - new_slab_objects 根据节点 id 找到对应 kmem_cache_node , 调用 get_partial_node
    - 首先从 kmem_cache_node 的 partial 链表拿下一大块内存, 替换 kmem_cache_cpu 的 page, 再取一块替换 kmem_cache_cpu 的 partial
    - 若 kmem_cache_node 也没有空闲, 则在 new_slab_objects 中调用 new_slab->allocate_slab->alloc_slab_page 根据某个 kmem_cache_order_objects 设置申请大块内存

- 页面换出

  - 触发换出:
    - 分配内存时发现没有空闲; 调用 `get_page_from_freelist->node_reclaim->__node_reclaim->shrink_node`
    - 内存管理主动换出, 由内核线程 kswapd 实现, kswapd 在内存不紧张时休眠, 在内存紧张时检测内存 调用 balance_pgdat->kswapd_shrink_node->shrink_node
  - 页面都挂在 lru 链表中, 页面有两种类型: 匿名页; 文件内存映射页
  - 每一类有两个列表: active 和 inactive 列表, 要换出时, 从 inactive 列表中找到最不活跃的页换出
  - 更新列表, shrink_list 先缩减 active 列表, 再缩减不活跃列表
  - 缩减不活跃列表时对页面进行回收:
    - 匿名页回收: 分配 swap, 将内存也写入文件系统
    - 文件内存映射页: 将内存中的文件修改写入文件中

![img](https://static001.geekbang.org/resource/image/52/54/527e5c861fd06c6eb61a761e4214ba54.jpeg)

对于物理内存来讲，从下层到上层的关系及分配模式如下：

1. 物理内存分 NUMA 节点，分别进行管理
2. 每个 NUMA 节点分成多个内存区域
3. 每个内存区域分成多个物理页面
4. 伙伴系统将多个连续的页面作为一个大的内存块分配给上层
5. kswapd 负责物理页面的换入换出
6. Slub Allocator 将从伙伴系统申请的大内存块切成小块，分配给其他系统

### 用户态内存映射

- 申请小块内存用 brk; 申请大块内存或文件映射用 mmap

- mmap 映射文件, 由 fd 得到 struct file，内存映射不仅仅是物理内存和虚拟内存之间的映射，还包括将文件中的内容映射到虚拟内存空间

  - 调用 ...->do_mmap
    - 调用 get_unmapped_area 找到一个可以进行映射的 vm_area_struct再调用 mmap_region 进行映射
  - get_unmapped_area
    - 匿名映射: 找到前一个 vm_area_struct
    - 文件映射: 调用 file 中 file_operations 文件的相关操作, 最终也会调用到 get_unmapped_area
  - mmap_region
    - 通过 vm_area_struct 判断, 能否基于现有的块扩展(调用 vma_merge)
    - 若不能, 调用 kmem_cache_alloc 在 slub 中得到一个 vm_area_struct 并进行设置
    - 若是文件映射：则调用 file_operations 的 mmap 将 vm_area_struct 的内存操作设置为文件系统对应操作(读写内存就是读写文件系统)，通过 vma_link 将 vm_area_struct 插入红黑树，调用 __vma_link_file 建立文件到内存的反映射

- 内存管理不直接分配内存, 在使用时才分配

- 用户态缺页异常, 触发缺页中断, 调用 do_page_default

- __do_page_fault 判断中断是否发生在内核

  - 若发生在内核, 调用 vmalloc_fault, 使用内核页表进行映射
  - 若不是, 找到对应 vm_area_struct 调用 handle_mm_fault

  ![img](https://static001.geekbang.org/resource/image/9b/f1/9b802943af4e3ae80ce4d0d7f2190af1.jpg)

  - 得到多级页表地址 pgd 等, pgd 存在 task_struct.mm_struct.pgd 中. pgd_t 用于全局页目录项，pud_t 用于上层页目录项，pmd_t 用于中间页目录项，pte_t 用于直接页表项
  - 全局页目录项 pgd 在创建进程 task_struct 时创建并初始化, 会调用 pgd_ctor 拷贝内核页表到进程的页表

- 进程被调度运行时, 通过 switch_mm_irqs_off->load_new_mm_cr3 切换内存上下文, cr3 是 cpu 寄存器, 存储进程 pgd 的物理地址(load_new_mm_cr3 加载时通过直接内存映射进行转换), cpu 访问进程虚拟内存时, 从 cr3 得到 pgd 页表, 最后得到进程访问的物理地址

- 进程地址转换发生在用户态, 缺页时才进入内核态(调用__handle_mm_fault)

- __handle_mm_fault 调用 pud_alloc, pmd_alloc创建相应的页目录项, handle_pte_fault 分配页表项

  - 若不存在 pte(页表项)
    - 匿名页: 调用 do_anonymous_page 分配物理页 ①
    - 文件映射: 调用 do_fault ②
  - 若存在 pte, 调用 do_swap_page 换入内存 ③
  - ① 为匿名页分配内存
    - 调用 pte_alloc 分配 pte 页表项
    - 调用 ...->__alloc_pages_nodemask 分配物理页
    - mk_pte 页表项指向物理页; set_pte_at 插入页表项
  - ② 为文件映射分配内存 __do_fault
    - 以 ext4 为例, 调用 ext4_file_fault->filemap_fault, 文件映射一般有物理页作为缓存 find_get_page 找缓存页
    - 若有缓存页, 调用函数预读数据到内存
    - 若无缓存页, 调用 page_cache_read 分配一个, 加入 lru 队列, 调用 readpage 读数据: 调用 kmap_atomic 将物理内存映射到内核临时映射空间, 由内核读取文件, 再调用 kunmap_atomic 解映射
    - 本来把物理内存映射到用户虚拟地址空间，不需要在内核里面映射一把。现在因为要从文件里面读取数据并写入这个物理页面，又不能使用物理地址，只能使用虚拟地址，就需要在内核里面临时映射
  - ③ do_swap_page
    - 先检查对应 swap 有没有缓存页
    - 没有, 读入 swap 文件(也是调用 readpage), 调用 mk_pte; set_pet_at; swap_free(清理 swap)

- 避免每次都需要经过页表(存再内存中)访问内存

  - TLB 缓存部分页表项的副本, 称为快表，专门用来做地址映射的硬件设备。它不在内存中，可存储的数据比较少，但是比内存要快。所以 TLB 就是页表的 Cache

![img](https://static001.geekbang.org/resource/image/94/b3/94efd92cbeb4d4ff155a645b93d71eb3.jpg)

用户态的内存映射机制包含以下几个部分：

- 用户态内存映射函数 mmap，包括用它来做匿名映射和文件映射

- 用户态的页表结构，存储位置在 mm_struct 中
- 在用户态访问没有映射的内存会引发缺页异常，分配物理页表、补齐页表
- 如果是匿名映射则分配物理内存；如果是 swap，则将 swap 文件读入；如果是文件映射，则将文件读入

![img](https://static001.geekbang.org/resource/image/78/44/78d351d0105c8e5bf0e49c685a2c1a44.jpg)

### 内核页表

```
- 内存映射函数 vmalloc, kmap_atomic
- 内核态页表存放位置和工作流程
- 内核态缺页异常处理
```

- 内核态页表, 系统初始化时就创建

  - swapper_pg_dir 指向内核顶级页目录 pgd
    - xxx_ident/kernel/fixmap_pgt 分别是直接映射/内核代码/固定映射的 xxx 级页表目录
  - 创建内核态页表
    - swapper_pg_dir 指向 init_top_pgt, 是 ELF 文件的全局变量, 因此在内存管理初始化之前就存在
    - init_top_pgt 先初始化了三项
      - 第一项指向 level3_ident_pgt (内核代码段的某个虚拟地址) 减去 __START_KERNEL_MAP (内核代码起始虚拟地址) 得到实际物理地址
      - 第二项也是指向 level3_ident_pgt
      - 第三项指向 level3_kernel_pgt 内核代码区

  ![img](https://static001.geekbang.org/resource/image/78/6d/78c8d44d7d8c08c03eee6f7a94652d6d.png)

  - 初始化各页表项, 指向下一集目录
    - 页表覆盖范围较小, 内核代码 512MB, 直接映射区 1GB
    - 内核态也定义 mm_struct 指向 swapper_pg_dir
    - 初始化内核态页表, start_kernel→ setup_arch
      - load_cr3(swapper_pg_dir) 并刷新 TLB
      - 调用 init_mem_mapping→kernel_physical_mapping_init, 用 __va 将物理地址映射到虚拟地址, 再创建映射页表项
      - CPU 在保护模式下访问虚拟地址都必须通过 cr3, 系统只能照做，在 load_cr3 之前, 通过 early_top_pgt 完成映射

- vmalloc 和 kmap_atomic

  - 内核的虚拟地址空间 vmalloc 区域用于映射
  - kmap_atomic 临时映射
    - 32 位, 调用 set_pte 通过内核页表临时映射
    - 64 位, 调用 page_address→lowmem_page_address 进行映射

- 内核态缺页异常

  - kmap_atomic 直接创建页表进行映射
  - vmalloc 只分配内核虚拟地址, 访问时触发缺页中断, 调用 do_page_fault→vmalloc_fault 用于关联内核页表项

- kmem_cache 和 kmalloc 用于保存内核数据结构, 不会被换出; 而内核 vmalloc 会被换出

整个内存管理的体系：

- 物理内存根据 NUMA 架构分节点。每个节点再分区域。每个区域再分页。物理页面通过伙伴系统进行分配。分配的物理页面要变成虚拟地址让上层可以访问，kswapd 可以根据物理页面的使用情况对页面进行换入换出
- 对于内存的分配需求，可能来自内核态，也可能来自用户态
- 对于内核态，kmalloc 在分配大内存的时候，以及 vmalloc 分配不连续物理页的时候，直接使用伙伴系统，分配后转换为虚拟地址，访问的时候需要通过内核页表进行映射。对于 kmem_cache 以及 kmalloc 分配小内存，则使用 slub 分配器，将伙伴系统分配出来的大块内存切成一小块一小块进行分配。kmem_cache 和 kmalloc 的部分不会被换出，因为用这两个函数分配的内存多用于保持内核关键的数据结构。内核态中 vmalloc 分配的部分会被换出，因而当访问的时候，发现不在，就会调用 do_page_fault
- 对于用户态的内存分配，或者直接调用 mmap 系统调用分配，或者调用 malloc。调用 malloc 的时候，如果分配小的内存，就用 sys_brk 系统调用；如果分配大的内存，还是用 sys_mmap 系统调用。正常情况下，用户态的内存都是可以换出的，因而一旦发现内存中不存在，就会调用 do_page_fault

![img](https://static001.geekbang.org/resource/image/27/9a/274e22b3f5196a4c68bb6813fb643f9a.png)