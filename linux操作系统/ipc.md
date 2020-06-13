### 进程间通信

##### 管道模型

匿名管道，没有名字，用完就销毁：

```
ps -ef | grep 关键字 | awk '{print $2}' | xargs kill -9
```

命名管道，需要通过 mkfifo 命令显式地创建：

```
// 创建命名管道
mkfifo hello

// 文件类型是pipe
# ls -l
prw-r--r--  1 root root         0 May 21 23:29 hello

// 一终端写入
# echo "hello world" > hello

// 另一终端读取
# cat < hello 
hello world
```

##### 消息队列模型

像邮件，发送数据时，会分成一个个独立的数据单元消息体，每个消息体都是固定大小的存储块，在字节流上不连续

```
struct msg_buffer {
    long mtype;
    char mtext[1024];
};

// 创建一个消息队列，使用 msgget 函数
// 指定一个文件，ftok 会根据这个文件的 inode，生成一个近乎唯一的 key
// ipcmk，ipcs 和 ipcrm 用于创建、查看和删除 IPC 对象
// 发送消息调用 msgsnd 函数
// 接收消息调用 msgrcv 函数
```

##### 共享内存模型

拿出一块虚拟地址空间来，映射到相同的物理内存中。这个进程写入的东西，另外一个进程马上就能看到了，不需要拷贝传递

```
// 创建一个 IPC 对象，第一个参数 key 同上
int shmget(key_t key, size_t size, int flag);

// 将这个内存加载到自己的虚拟地址空间的某个位置，attach
void *shmat(int shm_id, const void *addr, int flag);

// 通过 shmdt 解除绑定，然后通过 shmctl，将 cmd 设置为 IPC_RMID，从而删除这个共享内存对象
int shmdt(void *addr); 
int shmctl(int shm_id, int cmd, struct shmid_ds *buf);

// 通过 ipcs 命令查看这个共享内存
```

##### 信号量

如果两个进程 attach 同一个共享内存，都往里面写东西，很有可能就冲突了。信号量和共享内存往往要配合使用。信号量其实是一个计数器，主要用于实现进程间的互斥与同步，而不是用于存储进程间通信数据。可以将信号量初始化为一个数值，来代表某种资源的总体数量。会定义两种原子操作，一个是 P 操作，称为申请资源操作。这个操作会申请将信号量的数值减去 N，表示这些数量被他申请使用了，其他人不能用了。另一个是 V 操作，称为归还资源操作，这个操作会申请将信号量加上 M，表示这些数量已经还给信号量了，其他人可以使用了。

```
// 创建一个信号量组，第一个参数 key 同上，第二个参数 num_sems 不是指资源的数量，而是表示可以创建多少个信号量，形成一组信号量
int semget(key_t key, int num_sems, int sem_flags);

// 初始化信号量的总的资源数量。第一个参数 semid 是这个信号量组的 id，第二个参数 semnum 是在这个信号量组中某个信号量的 id，第三个参数是命令，如果是初始化，则用 SETVAL，第四个参数是一个 union。如果初始化，应该用里面的 val 设置资源总量
int semctl(int semid, int semnum, int cmd, union semun args);

union semun
{
  int val;
  struct semid_ds *buf;
  unsigned short int *array;
  struct seminfo *__buf;
};

// 无论是 P 操作还是 V 操作，第一个参数还是信号量组的 id，一次可以操作多个信号量。第三个参数 numops 就是有多少个操作，第二个参数将这些操作放在一个数组中
int semop(int semid, struct sembuf semoparray[], size_t numops);

struct sembuf 
{
  short sem_num; // 信号量组中对应的序号，0～sem_nums-1
  short sem_op;  // 信号量值在一次操作中的改变量
  short sem_flg; // IPC_NOWAIT, SEM_UNDO
}
```

##### 信号

可以在任何时候发送给某一进程，进程需要为信号配置信号处理函数。当信号发生的时候，就默认执行这个函数

```
// 查看所有的信号
kill -l

// 查看信号具体含义
man signal
```

信号的处理方式：

1. 执行默认操作。Linux 对每种信号都规定了默认操作，Term是终止进程。Core 是产生 Core Dump再终止进程
2. 捕捉信号。执行相应的信号处理函数
3. 忽略信号。有两个信号是应用进程无法捕捉和忽略的，即 SIGKILL 和 SEGSTOP

信号处理函数的定义方式：

1. signal，不推荐，signal 不是系统调用，而是 glibc 封装的一个函数

1. ```
   typedef void (*sighandler_t)(int);
   sighandler_t signal(int signum, sighandler_t handler);
   ```

2. sigaction ，推荐

   ```
   int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact);
   ```

信号的到来时间是不可预期的，有可能程序正在调用某个漫长的系统调用的时候一个信号来了，会中断这个系统调用，去执行信号处理函数，执行完了以后，两种处理方式：

1. SA_INTERRUPT，系统调用被中断了，直接返回一个 -EINTR 常量，用户自行处理
2. SA_RESTART，系统调用会被自动重新启动，不需要用户处理。可能存在问题，从终端读入一个字符，用户在终端输入一个'a'字符，在处理'a'字符的时候被信号中断了，等信号处理完毕，如果用户不再输入，就停在那里了，需要用户再次输入同一个字符

![img](https://static001.geekbang.org/resource/image/7c/28/7cb86c73b9e73893e6b0e0433d476928.png)

##### 信号的发送

中断要注册中断处理函数，但是中断处理函数是在内核驱动里面的，信号也要注册信号处理函数，信号处理函数是在用户态进程里面的。

```
kill->kill_something_info->kill_pid_info->group_send_sig_info->do_send_sig_info
tkill->do_tkill->do_send_specific->do_send_sig_info
tgkill->do_tkill->do_send_specific->do_send_sig_info
rt_sigqueueinfo->do_rt_sigqueueinfo->kill_proc_info->kill_pid_info->group_send_sig_info->do_send_sig_info
```

如果 kill 是发送给整个进程，应该 t->signal->shared_pending。是整个进程所有线程共享的信号；如果tkill 发送某个线程，应该 t->pending。是这个线程的 task_struct 独享的。struct sigpending 里面有两个成员

```
struct sigpending {
  struct list_head list;
  sigset_t signal;
};
```

不可靠信号：小于 32 的信号，信号来的太快，而sigset_t 集合中的已经存在，就会丢失

可靠信号：大于 32 的信号，通过 list_add_tail 挂在 struct sigpending 里面的链表list上。这样就不会丢，哪怕相同的信号发送多遍，也能处理多遍

当信号挂到了 task_struct 结构之后，最后调用 complete_signal，在找到了一个进程或者线程的 task_struct 之后，调用 signal_wake_up，来企图唤醒它，它主要做两件事：

1. 设置一个标识位 TIF_SIGPENDING，来表示已经有信号等待处理。等待系统调用结束，或者中断处理结束，从内核态返回用户态的时候，再进行信号的处理
2. 试图唤醒这个进程或者线程，将这个进程或者线程设置为 TASK_RUNNING，然后放在运行队列中

##### 信号的处理

无论是从系统调用返回还是从中断返回，都会调用 exit_to_usermode_loop，如果已经设置了 _TIF_SIGPENDING，就调用 do_signal->handle_signal 进行处理，信号处理就是调用用户提供的信号处理函数，但是并没有看起来简单，因为信号处理函数是在用户态

![img](https://static001.geekbang.org/resource/image/3d/fb/3dcb3366b11a3594b00805896b7731fb.png)

### 管道

##### 匿名管道

```
int pipe(int fd[2])
```

创建一个管道 pipe，返回两个文件描述符，表示管道的两端，一个是管道的读取端描述符 fd[0]，另一个是管道的写入端描述符 fd[1]。

1. __do_pipe_flags->create_pipe_files，生成了两个 fd。fd[0]用于读，fd[1]用于写，匿名管道，创建在文件系统上的，只不过是一种特殊的文件系统，创建一个特殊的文件，对应一个特殊的 inode，即get_pipe_inode，从其实现可以看出这是一个特殊的文件系统 pipefs
2. dentry 和 inode 对应好了，就开始创建 struct file 对象。先创建用于写入的 pipefifo_fops；再创建读取的也为 pipefifo_fops。然后把 private_data 设置为 pipe_inode_info。这样从 struct file 这个层级上，就能直接操作底层的读写操作
3. 一个匿名管道创建成功，对于 fd[1]写入，调用 pipe_write，向 pipe_buffer 里面写入数据；对于 fd[0]读入，调用 pipe_read，从 pipe_buffer 里面读取数据
4. 这两个文件描述符都是在一个进程里面，并没有起到进程间通信的作用，fork创建的子进程会复制父进程的 struct files_struct，fd 的数组会复制一份，但是 fd 指向的 struct file 对于同一个文件还是只有一份，这样就做到了，两个进程各有两个 fd 指向同一个 struct file 的模式，两个进程就可以通过各自的 fd 写入和读取同一个管道文件实现跨进程通信

![img](https://static001.geekbang.org/resource/image/71/b6/71eb7b4d026d04e4093daad7e24feab6.png)

通常的方法是父进程关闭读取的 fd，只保留写入的 fd，而子进程关闭写入的 fd，只保留读取的 fd，如果需要双向通行，则应该创建两个管道。

在 shell 里面运行 A|B 的时候，A 进程和 B 进程都是 shell 创建出来的子进程，A 和 B 之间不存在父子关系。首先从 shell 创建子进程 A，然后在 shell 和 A 之间建立一个管道，其中 shell 保留读取端，A 进程保留写入端，然后 shell 再创建子进程 B。这又是一次 fork，所以，shell 里面保留的读取端的 fd 也被复制到了子进程 B 里面。这个时候，相当于 shell 和 B 都保留读取端，只要 shell 主动关闭读取端，就变成了一管道，写入端在 A 进程，读取端在 B 进程。

![img](https://static001.geekbang.org/resource/image/81/fa/81be4d460aaa804e9176ec70d59fdefa.png)

接下来将这个管道的两端和输入输出关联起来：

在 A 进程中，写入端：dup2(fd[1],STDOUT_FILENO)，将 STDOUT_FILENO 不再指向标准输出，而是指向创建的管道文件，那么以后往标准输出写入的任何东西，都会写入管道文件

在 B 进程中，读取端：dup2(fd[0],STDIN_FILENO)，将 STDIN_FILENO 不再指向标准输入，而是指向创建的管道文件，那么以后从标准输入读取的任何东西，都来自于管道文件

![img](https://static001.geekbang.org/resource/image/c0/e2/c042b12de704995e4ba04173e0a304e2.png)

##### 命名管道

需要通过命令 mkfifo 进行创建。如果是通过代码创建命名管道，也有一个函数，但不是一个系统调用，而是 Glibc 提供的函数。

1. mkfifo 函数会调用 mknodat 系统调用，创建一个字符设备的时候，也调用 mknod。命名管道也是一个设备，也用 mknod。先是通过 user_path_create 对于这个管道文件创建一个 dentry，因为是 S_IFIFO，调用 vfs_mknod。由于这个管道文件是创建在一个普通文件系统上的，假设是在 ext4 文件上，vfs_mknod 会调用 ext4_dir_inode_operations 的 mknod，即 ext4_mknod。ext4_new_inode_start_handle 调用 __ext4_new_inode，在 ext4 文件系统上真的创建一个文件，调用 init_special_inode创建一个内存中特殊的 inode，这个函数在字符设备文件中也遇到过，当时 inode 的 i_fop 指向的是 def_chr_fops，这次换成管道文件了，inode 的 i_fop 变成指向 pipefifo_fops，这和匿名管道一样
2. 要打开这个管道文件，调用文件系统的 open 函数。沿着文件系统的调用方式，一路调用到 pipefifo_fops 的 open 函数，即 fifo_open。在 fifo_open 里面，创建 pipe_inode_info，这和匿名管道也一样。这个结构里面有个成员是 struct pipe_buffer *bufs。所谓的命名管道，其实是也是内核里面的一串缓存
3. 命名管道的写入，调用 pipefifo_fops 的 pipe_write ，向 pipe_buffer 里面写入数据
4. 命名管道的读入，调用 pipefifo_fops 的 pipe_read，从 pipe_buffer 里面读取数据

![img](https://static001.geekbang.org/resource/image/48/97/486e2bc73abbe91d7083bb1f4f678097.png)

### 共享内存的内核机制

消息队列、共享内存、信号量，它们在使用之前都要生成 key，然后通过 key 得到唯一的 id，并且都是通过 xxxget 函数。在内核里面，这三种进程间通信机制是使用统一的机制管理起来的， ipcxxx。为了维护这三种进程间通信进制，在内核里面，声明了一个有三项的数组

```
struct ipc_namespace {
......
  struct ipc_ids  ids[3];
......
}

#define IPC_SEM_IDS  0
#define IPC_MSG_IDS  1
#define IPC_SHM_IDS  2

#define sem_ids(ns)  ((ns)->ids[IPC_SEM_IDS])
#define msg_ids(ns)  ((ns)->ids[IPC_MSG_IDS])
#define shm_ids(ns)  ((ns)->ids[IPC_SHM_IDS])
```

in_use 表示当前有多少个 ipc；seq 和 next_id 用于一起生成 ipc 唯一的 id，ipcs_idr 是一棵基数树，sem_ids、msg_ids、shm_ids 各有一棵基数树

```
struct ipc_ids {
  int in_use;
  unsigned short seq;
  struct rw_semaphore rwsem;
  struct idr ipcs_idr;
  int next_id;
};

struct idr {
  struct radix_tree_root  idr_rt;
  unsigned int    idr_next;
};
```

完全可以通过 struct kern_ipc_perm 的指针，通过进行强制类型转换后，得到整个结构

![img](https://static001.geekbang.org/resource/image/08/af/082b742753d862cfeae520fb02aa41af.png)

共享内存的创建和映射过程：

1. 调用 shmget 创建共享内存
2. 先通过 ipc_findkey 在基数树中查找 key 对应的共享内存对象 shmid_kernel 是否已经被创建过，如果已经被创建，就会被查询出来，例如 producer 创建过，在 consumer 中就会查询出来
3. 如果共享内存没有被创建过，则调用 shm_ops 的 newseg 方法，创建一个共享内存对象 shmid_kernel。例如，在 producer 中就会新建
4. 在 shmem 文件系统里面创建一个文件，共享内存对象 shmid_kernel 指向这个文件，这个文件用 struct file 表示，称它为 file1
5. 调用 shmat，将共享内存映射到虚拟地址空间
6. shm_obtain_object_check 先从基数树里面找到 shmid_kernel 对象
7. 创建用于内存映射到文件的 file 和 shm_file_data，这里的 struct file 称为 file2
8. 关联内存区域 vm_area_struct 和用于内存映射到文件的 file，即 file2，调用 file2 的 mmap 函数
9. file2 的 mmap 函数 shm_mmap，会调用 file1 的 mmap 函数 shmem_mmap，设置 shm_file_data 和 vm_area_struct 的 vm_ops
10. 内存映射完毕之后，并没有真的分配物理内存，当访问内存的时候，会触发缺页异常 do_page_fault
11. vm_area_struct 的 vm_ops 的 shm_fault 会调用 shm_file_data 的 vm_ops 的 shmem_fault
12. 在 page cache 中找一个空闲页，或者创建一个空闲页

![img](https://static001.geekbang.org/resource/image/20/51/20e8f4e69d47b7469f374bc9fbcf7251.png)

### 信号量的内核机制

![img](https://static001.geekbang.org/resource/image/60/7c/6028c83b0aa00e65916988911aa01b7c.png)

其主要流程：

1. 调用 semget 创建信号量集合
2. ipc_findkey 会在基数树中，根据 key 查找信号量集合 sem_array 对象。如果已经被创建，就会被查询出来
3. 如果信号量集合没有被创建过，则调用 sem_ops 的 newary 方法，创建一个信号量集合对象 sem_array
4. 调用 semctl(SETALL) 初始化信号量
5. sem_obtain_object_check 先从基数树里面找到 sem_array 对象
6. 根据用户指定的信号量数组，初始化信号量集合，即初始化 sem_array 对象的 struct sem sems[]成员
7. 调用 semop 操作信号量
8. 创建信号量操作结构 sem_queue，放入队列
9. 创建 undo 结构，放入链表

