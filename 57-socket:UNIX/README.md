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

