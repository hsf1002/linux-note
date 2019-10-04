### 第57章 SOCKET: UNIX DOMAIN

##### UNIX domain socket的地址：struct sockaddr_un

```
struct socketaddr_un
{
    sa_family_t  sun_family: /* Always AF_UNIX */
    char sun_path[108];			/* Null-terminated socket pathname*/
}
// sun表示Socket unix
```

绑定UNIX domain时，bind会在文件系统中创建一个条目（因此作为socket路径名的一部分的目录需要可访问和可写），绑定的规则：

* 无法将一个socket绑定到几个既有路径上
* 通常会将一个socket绑定到绝对路径上，也可是相对路径，但客户端connect时需要知道该路径
* 一个socket只能绑定到一个路径，一个路径名只能被一个socket绑定
* 无法使用open打开一个socket
* 当不再需要一个socket时可以使用unlink或remove删除路径条目

应该把UNIX domain socket绑定到一个公共的有安全保护措施的绝对路径上

##### UNIX socket中的流socket

-nothing-

##### UNIX socket中的数据报

对于UNIX domain socket来说，数据报的传输时在内核中发生，不仅是可靠的，而且会按顺序接收，不会重复

##### UNIX domain socket权限

socket文件的所有权和权限决定了哪些进程能够与这个socket进行通信，默认情况下，创建socket（通过bind）时会给所有者、组和other用户赋予所有的权限，要改变这种行为可以在调用bind前用umask禁用不希望赋予的权限

##### sockpair(): 创建互联socket对

有时候单个进程创建一对socket并将它们连接起来比较有用，sockpair提供了一种快捷方式：

```
#include <sys/socket.h>
 
int socketpair(int domain, int type, int protocol, int fd[2]);
// 返回值：若成功，返回0，若出错，返回-1
// domain只能指定为AF_UNIX，type可以是SOCK_DGRAM或SOCK_STREAM，protocol必须是0，fd数组返回了引用这两个互相连接的socket的文件描述符
// 将type指定为SOCK_STREAM，相当于之间一个双向管道，每个socket都可以用来读写，并且这两个socket之间每个方向上的数据信道是分开的
// 使用socketpair创建的一对socket不会绑定到任何地址，它们对于其他进程完全不可见
```

##### Linux抽象socket名空间

抽象路径名空间是Linux特有的，它允许将一个UNIX domain socket绑定到一个名字但不会在文件系统中创建该名字，有如下优势：

* 无须担心与文件系统既有名称产生冲突
* 没必要在使用完socket后删除socket路径名，socket被关闭后会自动删除这个抽象名
* 无需为socket创建一个文件系统路径名，对于不具备文件系统写权限时比较有用

要创建一个抽象绑定，需要将sun_path字段的第一个字节指定为NULL

```
void create_virtual_socket()
{
    int sockfd;
    struct sockaddr_un addr;
    memset(&addr, 0x00, sizeof(struct sockaddr_un));

    // addr.sun_path[0] has been set to 0 by memset
    strncpy(&addr.sun_path[1], "xyz", sizeof(addr.sun_path) - 2);
    if (-1 == (sockfd = socket(AF_UNIX, SOCK_STREAM, 0)))
        perror("socket error");
    if (-1 == bind(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)))
        perror("bind error");
}
```

