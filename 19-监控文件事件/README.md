### 第19章 监控文件事件

##### 概述

inotify和dnotify都是Linux的专有机制，可以对文件或目录进行监控，侦测其是否发生了特定事件，关键步骤：

1. 使用inotify_init创建inotify实例，返回其文件描述符
2. 使用inotify_add_watch向inotify实例中添加监控列表
3. 对inotify的文件描述符执行read，以获取一个或多个通知事件
4. 结束监控时关闭inotify文件描述符

inotify监控机制非递归，但当监控目录时，与路径自身及所含文件相关的事件都会通知给程序，除了read，还可以使用select、poll、epoll以及由信号驱动的IO来监控；inotify机制时可选的内核组件，通过CONFIG_INOTIFY和CONFIG_INOTIFY_USER选项进行配置

##### inotify API

inotify_init用来创建一个新的inotify实例：

```
#include <sys/inotify.h>

int inotify_init(void);
// 返回值：若成功，返回文件描述符（用以指代inotify实例），若出错，返回-1
// 非标准的系统调用inotify_init1支持两个标记：IN_CLOEXEC和IN_NONBLOCK
```

inotify_add_watch既可以追加新的监控项，又可以修改现有监控项：

```
int inotify_add_watch(int fd, const char *pathname, uint32_t mask);
// 返回值：若成功，返回监控的描述符，若出错，返回-1
// 调用程序必须对该文件具有读权限
// 如果未将pathname加入，则创建一个新的监控项，并返回新的非负的监控描述符，指代此监控项，对inotify实例来说，监控描述符是唯一的
// 若干已假如pathname，则修改现有监控项的掩码mask，并返回其监控描述符
```

inotify_rm_watch删除由wd定义的监控项：

```
int inotify_rm_watch(int fd, uint32_t wd);
// 返回值：若成功，返回0，若出错，返回-1
// wd是监控描述符
```

