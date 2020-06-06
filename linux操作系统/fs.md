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