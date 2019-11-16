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

##### inotify事件

```
IN_ACCESS				文件被访问
IN_ATTRIB				文件元数据（权限、所有权、链接计数、扩展属性、用户ID或组ID）改变
IN_CLOSE_WRITE	关闭为了写入而打开的文件
IN_CREATE				在受监控目录下创建了文件或目录
IN_DELETE				在受监控目录内删除了文件或目录
IN_DELETE_SELF	删除了受监控目录/文件本身
IN_MODIFY				文件被修改
IN_MODIFY_SELF	移动受监控目录或文件本身
IN_MOVED_FROM		文件移除受监控目录
IN_MOVED_TO			将文件移到受监控目录
IN_OPEN					文件被打开
IN_ALL_EVENTS		以上所有输出事件的统称
IN_MOVE				  IN_MOVED_FROM | IN_MOVED_TO事件的统称
IN_ONESHOT			只监控pathname的一个事件
IN_ONLYDIR			pathname不为目录时会失败
......
```

##### 读取inotify事件

可用read读取事件，若未发生任何事件，则一直阻塞，直到有事件发生，事件发生后，read会返回一个缓冲区，内含一个或多个如下结构：

```
struct inotify_event
{
	int wd;					// 监控描述符
	uint32_t mask;  // 掩码事件，移除监控项时，产生IN_IGNORED
	uint32_t cookie:// 重命名时才用，待重命名文件所在目录产生IN_MOVED_FROM，命名后文件所在目录产生IN_MOVED_TO（若在同一目录重命名文件，则同时产生这两个事件），两个事件的cookie字段值相等
	uint32_t len;   // name的长度，给read缓冲区的大小至少为sizeof(struct inotify_evnet) + NAME_MAX（文件名的最大长度） + 1
	char name[];    // 可选的null结尾的字符串，以标识发生事件的文件
}
```

##### 队列限制和/proc文件

对inotify事件排队，需要消耗内核内存，内核会对其限制，超级用户配置/proc/sys/fs/inotify路径下的三个文件可调整：

* max_queued_events：调用inotify_init时，该值为新inotify实例队列的事件设置上限
* max_user_instances：对由每个真实用户ID创建的inotify实例数的限制值
* max_user_watches：对由每个真实用户ID创建的监控项数量的限制值

##### 监控文件的旧有系统：dnotify

问世于2.4，如今已落伍，有如下局限：

1. dnotify通过向应用程序发送信号来通告事件
2. dnotify的监控单元只能是目录
3. dnotify需要应用程序为该目录打开文件描述符
4. dnotify提供的与文件事件相关的信息不够准确
5. 某些情况下，dnotify不支持可靠的文件事件通告机制

