## 第46章 System V 消息队列

### 创建或打开一个消息队列

```
struct msqid_ds 
{
    struct ipc_perm msg_perm;	/* see Section 15.6.2 */
    struct msg *msg_first;      /* first message on queue,unused  */
    struct msg *msg_last;       /* last message in queue,unused */
    __kernel_time_t msg_stime;  /* last msgsnd time */
    __kernel_time_t msg_rtime;  /* last msgrcv time */
    __kernel_time_t msg_ctime;  /* last change time */
    unsigned long  msg_lcbytes; /* Reuse junk fields for 32 bit */
    unsigned long  msg_lqbytes; /* ditto */
    unsigned short msg_cbytes;  /* current number of bytes on queue */
    unsigned short msg_qnum;    /* number of messages in queue */
    unsigned short msg_qbytes;  /* max number of bytes on queue */
    __kernel_ipc_pid_t msg_lspid;   /* pid of last msgsnd */
    __kernel_ipc_pid_t msg_lrpid;   /* last receive pid */
};
```

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

