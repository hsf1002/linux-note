### 文件系统

##### 功能规划

1. 文件系统要有严格的组织形式，使得文件能够以块为单位进行存储
2. 文件系统中要有索引区，用来方便查找一个文件分成的多个块都存放在了什么位置
3. 如果文件系统中有的文件是热点文件，近期经常被读取和写入，文件系统应该有缓存层
4. 文件应该用文件夹的形式组织起来，方便管理和查询，还要有相对路径和绝对路径
5. Linux 内核要在自己的内存里面维护一套数据结构，来保存哪些文件被哪些进程打开和使用

##### 相关命令行

当一个 Linux 系统插入了一块没有格式化的硬盘的时候，通过命令 fdisk -l，查看格式化和没有格式化的分区，可以通过命令 mkfs.ext3 或者 mkfs.ext4 进行格式化

```
mkfs.ext4 /dev/vdc
```

可以选择不将整块盘格式化为一个分区，而是格式化为多个分区。下面的这个命令行可以启动一个交互式程序

```
fdisk /dev/vdc
```

格式化后的硬盘，需要挂在到某个目录下面，才能作为普通的文件系统进行访问

```
mount /dev/vdc1 /根目录/用户A目录/目录1
```

卸载使用 umount 命令

```
umount /根目录/用户A目录/目录1
```

##### 相关接口

```
系统调用：open/lseek/stat/read/write/close
库函数：  opendir/readdir/closedir
```

### 磁盘系统

##### inode 与块的存储

磁盘有多个盘片，每个盘片有多个磁道，每个磁道分多个扇区，每个扇区是 512 个字节，硬盘分成相同大小的单元，称为块（Block）。一块的大小是扇区大小的整数倍，默认是 4K。在格式化的时候，这个值是可以设定

inode 的“i”是 index 的意思，其实就是“索引”，可以存放元数据

```
struct ext4_inode {
  __le16  i_mode;    /* File mode */
  __le16  i_uid;    /* Low 16 bits of Owner Uid */
  __le32  i_size_lo;  /* Size in bytes */
  __le32  i_atime;  /* Access time */
  __le32  i_ctime;  /* Inode Change time */
  __le32  i_mtime;  /* Modification time */
  __le32  i_dtime;  /* Deletion Time */
  __le16  i_gid;    /* Low 16 bits of Group Id */
  __le16  i_links_count;  /* Links count */
  __le32  i_blocks_lo;  /* Blocks count */
  __le32  i_flags;  /* File flags */
......
  __le32  i_block[EXT4_N_BLOCKS];/* Pointers to blocks */
  __le32  i_generation;  /* File version (for NFS) */
  __le32  i_file_acl_lo;  /* File ACL */
  __le32  i_size_high;
......
};
```

“某个文件分成几块、每一块在哪里”，在 inode 里面，应该保存在 i_block 里面，在 ext2 和 ext3 中，其中前 12 项直接保存了块的位置，可以通过 i_block[0-11]，直接得到保存文件内容的块。如果一个文件比较大，12 块放不下，可以让 i_block[12]指向一个块，这个块里面不放数据块，而是放数据块的位置，这个块称为间接块

![img](https://static001.geekbang.org/resource/image/73/e2/73349c0fab1a92d4e1ae0c684cfe06e2.jpeg)

如果文件再大一些，可以用二次间接块、三次间接块，对于大文件来讲，要多次读取硬盘才能找到相应的块，访问速度就会比较慢。为了解决这个问题，ext4 引入了一个新的概念，叫做 Extents，它会保存为一棵树

![img](https://static001.geekbang.org/resource/image/b8/2a/b8f184696be8d37ad6f2e2a4f12d002a.jpeg)

一个文件大小为 128M，如果使用 4k 大小的块进行存储，需要 32k 个块。如果按照 ext2 或者 ext3 那样散着放，数量太大。但是 Extents 可以用于存放连续的块，可以把 128M 放在一个 Extents 里面。对大文件的读写性能提高了，文件碎片也减少了

eh_entries 表示这个节点里面有多少项。这里的项分两种，如果是叶子节点，这一项会直接指向硬盘上的连续块的地址，为数据节点 ext4_extent；如果是分支节点，这一项会指向下一层的分支节点或者叶子节点，为索引节点

##### inode 位图和块位图

要保存一个 inode，应该放在硬盘上的哪个位置呢？不能将所有的 inode 列表和块列表扫描一遍，可以用一个块来保存 inode 的位图。在这 4k 里面，每一位对应一个 inode。如果是 1，表示这个 inode 已经被用了；如果是 0，则表示没被用。同样，也有一个块保存 block 的位图。从文件系统里面读取 inode 位图，然后找到下一个为 0 的 inode，就是空闲的 inode

##### 文件系统的格式

![img](https://static001.geekbang.org/resource/image/e3/1b/e3718f0af6a2523a43606a0c4003631b.jpeg)

* 块组：“一个块的位图 + 一系列的块” + “一个块的 inode 的位图 + 一系列的 inode 的结构”，最多能够表示 128M。太小了，现在很多文件都比这个大。把这个结构称为一个块组。有 N 多的块组，就能够表示 N 大的文件

* 块组描述符表：因为块组有多个，块组描述符也同样组成一个列表

* 超级块：对整个文件系统的情况进行描述，整个文件系统一共有多少 inode，s_inodes_count；一共有多少块，s_blocks_count_lo，每个块组有多少 inode，s_inodes_per_group，每个块组有多少块，s_blocks_per_group 等
* 引导区

* Meta Block Groups：块组描述符表不会保存所有块组的描述符了，而是将块组分成多个组，为元块组（Meta Block Group）。每个元块组里面的块组描述符表仅仅包括自己的，一个元块组包含 64 个块组，这样一个元块组中的块组描述符表最多 64 项

![img](https://static001.geekbang.org/resource/image/b0/b9/b0bf4690882253a70705acc7368983b9.jpeg)

##### 目录的存储格式

其实目录本身也是个文件，也有 inode。inode 里面也是指向一些块。和普通文件不同的是，普通文件的块里面保存的是文件数据，而目录文件的块里面保存的是目录里面一项一项的文件信息。称为 ext4_dir_entry。

如果在 inode 中设置 EXT4_INDEX_FL 标志，则目录文件的块的组织形式将发生变化。要查找一个目录下面的文件名，可以通过名称取哈希。如果哈希能够匹配上，就说明这个文件的信息在相应的块里面。然后打开这个块，如果里面不再是索引，而是索引树的叶子节点的话，那里面还是 ext4_dir_entry_2 的列表，只要一项一项找文件名就行。通过索引树，我们可以将一个目录下面的 N 多的文件分散到很多的块里面，可以很快地进行查找

![img](https://static001.geekbang.org/resource/image/3e/6d/3ea2ad5704f20538d9c911b02f42086d.jpeg)

##### 软链接和硬链接的存储格式

ln -s 创建的是软链接，不带 -s 创建的是硬链接

```
ln [参数][源文件或目录][目标文件或目录]
```

![img](https://static001.geekbang.org/resource/image/45/7b/45a6cfdd9d45e30dc2f38f0d2572be7b.jpeg)

* 硬链接：与原始文件共用一个 inode ，但是 inode 是不跨文件系统的，每个文件系统都有自己的 inode 列表，因而硬链接是没有办法跨文件系统的
* 软链接：相当于重新创建了一个文件。也有独立的 inode，打开这个文件看里面内容的时候指向另外的一个文件。可以跨文件系统，甚至目标文件被删除了，链接文件还是在的，只不过指向的文件找不到了而已

![img](https://static001.geekbang.org/resource/image/f8/38/f81bf3e5a6cd060c3225a8ae1803a138.png)

### 虚拟文件系统

![img](https://static001.geekbang.org/resource/image/3c/73/3c506edf93b15341da3db658e9970773.jpg)

##### 挂载文件系统

内核是不是支持某种类型的文件系统，需要进行注册register_filesystem才能知道，如果一种文件系统的类型曾经在内核注册过，这就说明允许挂载并且使用这个文件系统，调用链为：do_mount->do_new_mount->vfs_kern_mount

![img](https://static001.geekbang.org/resource/image/66/27/663b3c5903d15fd9ba52f6d049e0dc27.jpeg)

1. 第一层：根目录 / 对应一个 dentry（虽然叫作 directory entry，但不仅仅表示文件夹，也表示文件），根目录是在根文件系统上的，根文件系统是系统启动的时候挂载的，因而有一个 mount 结构。这个 mount 结构的 mount point 指针和 mount root 指针都是指向根目录的 dentry。根目录对应的 file 的两个指针，一个指向根目录的 dentry，一个指向根目录的挂载结构 mount
2. 第二层：目录 home 对应了两个 dentry，它们的 parent 都指向第一层的 dentry。因为文件系统 A 挂载到了这个目录下。这个目录有两个用处。一方面，home 是根文件系统的一个挂载点；另一方面，home 是文件系统 A 的根目录，因为还有一次挂载，又有了一个 mount 结构。 mount point 指针指向作为挂载点的那个 dentry。mount root 指针指向作为根目录的那个 dentry，同时 parent 指针指向第一层的 mount 结构。home 对应的 file 的两个指针，一个指向文件系统 A 根目录的 dentry，一个指向文件系统 A 的挂载结构 mount
3. 第三层：目录 hello 又挂载了一个文件系统 B，第三层的结构和第二层几乎一样
4. 第四层：目录 world 就是一个普通的目录。只要它的 dentry 的 parent 指针指向上一层就可以了。world 对应的 file 结构。由于挂载点不变，还是指向第三层的 mount 结构
5. 第五层：对于文件 data，是一个普通的文件，它的 dentry 的 parent 指向第四层的 dentry。对于 data 对应的 file 结构，由于挂载点不变，还是指向第三层的 mount 结构

##### 打开文件

do_sys_open

1. 首先要通过 get_unused_fd_flags 得到一个没有用的文件描述符

2. do_sys_open 中调用 do_filp_open，就是创建这个 struct file 结构，然后 fd_install(fd, f) 是将文件描述符和这个结构关联起来

3. 接下来就调用 path_openat，主要做了以下几件事情

   * get_empty_filp 生成一个 struct file 结构
   * path_init 初始化 nameidata，准备开始节点路径查找
   * link_path_walk 对于路径名逐层进行节点路径查找，这里面有一个大的循环，用“/”分隔逐层处理
   * do_last 获取文件对应的 inode 对象，并且初始化 file 对象

   例如，文件“/root/hello/world/data”，link_path_walk 会解析前面的路径部分“/root/hello/world”，解析完毕的时候 nameidata 的 dentry 为路径名的最后一部分的父目录“/root/hello/world”，而 nameidata 的 filename 为路径名的最后一部分“data”

4. do_last需要先查找文件路径最后一部分对应的 dentry。Linux 为了提高目录项对象的处理效率，设计与实现了目录项高速缓存 dentry cache，简称 dcache。主要由两个数据结构组成

   * 哈希表 dentry_hashtable：dcache 中的所有 dentry 对象都通过 d_hash 指针链到相应的 dentry 哈希链表中
   * 未使用的 dentry 对象链表 s_dentry_lru：dentry 对象通过其 d_lru 指针链入 LRU 链表中。有它，就说明长时间不使用，就应该释放了

![img](https://static001.geekbang.org/resource/image/82/59/82dd76e1e84915206eefb8fc88385859.jpeg)

这两个列表之间会产生复杂的关系：

* 引用为 0：一个在散列表中的 dentry 变成没有引用了，就会被加到 LRU 表中去
* 再次被引用：一个在 LRU 表中的 dentry 再次被引用了，则从 LRU 表中移除
* 分配：当 dentry 在散列表中没有找到，则从 Slub 分配器中分配一个
* 过期归还：当 LRU 表中最长时间没有使用的 dentry 应该释放回 Slub 分配器
* 文件删除：文件被删除了，相应的 dentry 应该释放回 Slub 分配器
* 结构复用：当需要分配一个 dentry，但是无法分配新的，就从 LRU 表中取出一个来复用

do_last() 在查找 dentry 的时候，先从缓存中查找，调用的是 lookup_fast。如果缓存中没有找到，就需要真的到文件系统里面去找，lookup_open 会创建一个新的 dentry，并且调用上一级目录的 Inode 的 inode_operations 的 lookup 函数，对于 ext4 来讲，调用的是 ext4_lookup

5. do_last() 的最后一步是调用 vfs_open 真正打开文件

![img](https://static001.geekbang.org/resource/image/80/b9/8070294bacd74e0ac5ccc5ac88be1bb9.png)

### 文件缓存

##### 系统调用层和虚拟文件系统层

` read -> vfs_read -> __vfs_read,  write -> vfs_write->__vfs_write`

清除缓存：

```
sync; echo 1 > /proc/sys/vm/drop_caches
```

##### ext4 文件系统层

`ext4_file_read_iter -> generic_file_read_iter，ext4_file_write_iter -> __generic_file_write_iter`

根据是否使用内存做缓存，可以把文件的 I/O 操作分为两种类型：

* 第一种类型是缓存 I/O：大多数文件系统的默认 I/O 操作都是缓存 I/O。对于读操作来讲，操作系统会先检查，内核的缓冲区有没有需要的数据。如果已经缓存了，那就直接从缓存中返回；否则从磁盘中读取，然后缓存在操作系统的缓存中。对于写操作来讲，操作系统会先将数据从用户空间复制到内核空间的缓存中。这时对用户程序来说，写操作就已经完成。至于什么时候再写到磁盘中由操作系统决定，除非显式地调用了 sync 同步命令
* 第二种类型是直接 IO：应用程序直接访问磁盘数据，而不经过内核缓冲区，从而减少了在内核缓存和用户程序之间数据复制

##### 带缓存的写入操作

generic_perform_write是一个 while 循环。找出这次写入影响的所有的页，然后依次写入。对于每一个循环，主要做四件事情：

1. 对于每一页，先调用 address_space 的 write_begin 做一些准备
   * 日志**（Journal）**模式：日志文件系统比非日志文件系统多了一个 Journal 区域。文件在 ext4 中分两部分存储，一部分是文件的元数据，另一部分是数据。元数据和数据的操作日志 Journal 也是分开管理的。可以在挂载 ext4 的时候，选择 Journal 模式。这种模式在将数据写入文件系统前，必须等待元数据和数据的日志已经落盘才能发挥作用。性能比较差，但是最安全
   * order 模式：不记录数据的日志，只记录元数据的日志，但是在写元数据的日志前，必须先确保数据已经落盘。这个折中，是默认模式
   * writeback：不记录数据的日志，仅记录元数据的日志，并且不保证数据比元数据先落盘。性能最好，但是最不安全
2. 调用 iov_iter_copy_from_user_atomic，将写入的内容从用户态拷贝到内核态的页中
3. 调用 address_space 的 write_end 完成写操作
4. 调用 balance_dirty_pages_ratelimited，看脏页是否太多，需要写回硬盘。所谓脏页，就是写入到缓存，但是还没有写入到硬盘的页面

触发回写的场景：

1. 调用 write 的最后，发现缓存的数据太多
2. 用户主动调用 sync，将缓存刷到硬盘上去，最终会调用 wakeup_flusher_threads，同步脏页
3. 当内存十分紧张，以至于无法分配页面的时候，会调用 free_more_memory，最终会调用 wakeup_flusher_threads，释放脏页
4. 脏页已经更新了较长时间，时间上超过了 timer，需要及时回写，保持内存和磁盘上数据一致性

##### 带缓存的读操作

generic_file_buffered_read，需要先找到 page cache 里面是否有缓存页。如果没有找到，不但读取这一页，还要进行预读，需要在 page_cache_sync_readahead 函数中实现。预读完了以后，再试一把查找缓存页，应该能找到了。如果第一次找缓存页就找到了，还是要判断，是不是应该继续预读；如果需要，就调用 page_cache_async_readahead 发起一个异步预读。最后，copy_page_to_iter 会将内容从内核缓存页拷贝到用户内存空间

![img](https://static001.geekbang.org/resource/image/0c/65/0c49a870b9e6441381fec8d9bf3dee65.png)

在系统调用层调用 read 和 write。在 VFS 层调用的是 vfs_read 和 vfs_write 并且调用 file_operation。在 ext4 层调用的是 ext4_file_read_iter 和 ext4_file_write_iter。接下来就是分叉。缓存 I/O 和直接 I/O。直接 I/O 读写的流程是一样的，调用 ext4_direct_IO，再往下就调用块设备层。缓存 I/O 读写的流程不一样。对于读，从块设备读取到缓存中，然后从缓存中拷贝到用户态。对于写，从用户态拷贝到缓存，设置缓存页为脏，然后启动一个线程写入块设备