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