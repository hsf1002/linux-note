## 第46章 System V 消息队列

### 数据结构

```
struct msqid_ds 
{
    struct ipc_perm msg_perm;	  /* 其中的uid、gid、mode可以通过IPC_SET修改 */
    struct msg *msg_first;      /* 第一条消息，未使用  */
    struct msg *msg_last;       /* 最后一条消息，未使用 */
    __kernel_time_t msg_stime;  /* 初始值为0，调用msgsnd后更新为当前时间 */
    __kernel_time_t msg_rtime;  /* 初始值为0，调用msgrcv后更新为当前时间 */
    __kernel_time_t msg_ctime;  /* 创建或调用IPC_SET后更新为当前时间 */
    unsigned long  msg_lcbytes; /* Reuse junk fields for 32 bit */
    unsigned long  msg_lqbytes; /* ditto */
    unsigned short msg_cbytes;  /* 调用msgsnd或msgrcv后后更新 */
    unsigned short msg_qnum;    /* 消息队列当前总数，调用msgsnd后递增，调用msgrcv后递减 */
    unsigned short msg_qbytes;  /* 消息队列所有消息的mtext字段的大小总和 */
    __kernel_ipc_pid_t msg_lspid;   /* 初始值为0，调用msgsnd后更新为调用进程的PID */
    __kernel_ipc_pid_t msg_lrpid;   /* 初始值为0，调用msgrcv后更新为调用进程的PID */
};
```

### 创建或打开一个消息队列

```
#include <sys/msg.h>

int msgget(key_t key, int flag);
// 若成功，返回消息队列ID，若出错，返回-1

flag代表的是权限和IPC_CREAT、IPC_EXCL的取或
用户读        0400
用户写(更改)   0200
组读          0040
组写(更改)     0020
其他读         0004
其他写(更改)    0002

如果创建新队列，需要初始化msqid_ds的以下成员：
msg_perm：该结构中的mode成员按照flag的权限位设置
msg_qnum、msg_lspid、msg_lrpid、msg_stime、msg_rtime为0
msg_ctime：设置为当前时间
msg_qbytes：设置为系统限制值
```

### 发送消息

```
struct mymsg
{
    long mtype;
    char mtext[512];
}
```

```
int msgsnd(int msqid, const void *ptr, size_t nbytes, int flag);
// 若成功，返回0，若出错，返回-1
// msgsnd若成功返回，消息队列相关的msqid_ds结构会随之更新

每个消息包含三部分：一个正的长整型字段、一个非负长度nbytes以及实际数据，总是放在队列尾端
flag：可以指定为IPC_NOWAIT，类似于文件IO的非阻塞标志
ptr：是一个mymsg的结构
```

### 读取消息

```
ssize_t msgrcv(int msqid, void *ptr, size_t nbytes, long type, int flag);
// 若成功，返回消息数据部分的长度，若出错，返回-1

和msgsnd一样，ptr指向一个长整型数，其后是实际消息数据的缓冲区
nbytes：缓冲区的长度
flag：
    MSG_NOERROR：若消息长度大于nbytes，且flag设置了MSG_NOERROR，消息会被截断，不设置此标志位，则出错返回E2BIG，
    MSG_NOWAIT：执行非阻塞接收
    MSG_EXCEPT：只有当type大于0才起作用，将队列中第一条mtype不等于type的消息删除并将其返回给调用者
type：返回哪一种消息，非0来取非先进先出的消息
    type==0：返回队列第一条消息
    type>0：返回队列消息类型是type的第一条消息
    type<0：返回消息队列中消息类型值小于等于type绝对值的消息，如果由多个，取类型值最小的消息
```

### 控制操作

```
int msgctl(int msqid, int cmd, struct msgid_ds *buf);
// 若成功，返回0，若出错，返回-1

cmd参数指定对msgid的队列要执行的命令，这三条命令也适用于信号量和共享存储
IPC_STAT：取队列的msqid_ds结构，放在buf中
IPC_SET：将字段msg_perm.uid、msg_perm.gid、msg_perm.mode和msg_qbytes从buf指向的结构复制到msqid
IPC_RMID：删除消息队列及其数据，立刻生效，队列中剩余消息都会丢失，所有被阻塞的读者和写者进程会立刻醒来，忽略第三个参数
```

### 消息队列的限制

* MSGMNI：系统级，所能创建的消息队列标识符的数量（即消息队列的个数）
* MSGMAX：系统级，单条消息最多可写入的字节数（msgsnd，EINVAL）
* MSGMNB：系统级，一个消息队列中一次最多可以保存的字节数（msg_qbytes）
* MSGTQL：系统级，所有消息队列所能存放的消息总数
* MSGPOLL：系统级，所有消息队列的数据的缓冲池的大小

Linux特有的msgctl IPC_INFO操作能够获取一个类型为msginfo的结构，其中包含了各种消息队列的限制值