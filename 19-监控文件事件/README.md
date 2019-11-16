### 第19章 监控文件事件

##### 概述

inotify和dnotify都是Linux的专有机制，可以对文件或目录进行监控，侦测其是否发生了特定事件，关键步骤：

1. 使用inotify_init创建inotify实例，返回其文件描述符
2. 使用inotify_add_watch向inotify实例中添加监控列表
3. 对inotify的文件描述符执行read，以获取一个或多个通知事件
4. 结束监控时关闭inotify文件描述符

inotify监控机制非递归，但当监控目录时，与路径自身及所含文件相关的事件都会通知给程序，除了read，还可以使用select、poll、epoll以及由信号驱动的IO来监控；inotify机制时可选的内核组件，通过CONFIG_INOTIFY和CONFIG_INOTIFY_USER选项进行配置



