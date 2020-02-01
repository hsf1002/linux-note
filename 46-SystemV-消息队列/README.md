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