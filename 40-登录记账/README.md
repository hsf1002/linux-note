## 第40章 登录记账

### utmp和wtmp概述

utmp：维护着当前登录进系统的用户记录，记录中包含ut_user字段，用户登出时删除这条记录，如who

wtmp：包含所有用户登录和登出的记录，登录时在写入utmp的同时写入wtmp相同记录，登出出再写入wtmp一条记录，如last

在Linux中，utmp文件位于/var/run/utmp，wtmp文件位于/var/log/wtmp，程序中应使用`_PATH_UTMP`和`_PATH_WTMP`

### utmpx API

Linux提供了传统的utmp和从System V演化而来的utmpx API，这两组API返回的信息是完全一样的，差别之一是utmp API中的一些函数时可重入的，而utmpx中的函数时不可重入的

