### 第9章 进程凭证

##### 概述

每个进程都有一套用数字表示的用户ID，组ID，有时候将这些ID称之为进程凭证：

* 实际用户ID（real user ID）和实际组ID（real group ID）
* 有效用户ID（effective user ID）和有效组ID（effective group ID）
* 保存的set-user-id（saved set-user-ID）和保存的set-group-id（saved set-group-ID）
* 文件系统用户ID（file-system user ID）和文件系统组ID（file-system group ID）
* 辅助组ID

