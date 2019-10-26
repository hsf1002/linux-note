### 第4章 文件IO：通用的IO模型

##### 概述

所有执行IO操作的系统调用都会以文件描述符，一个非负整数，来指代一个打开的文件，包括管道、FIFO、socket、终端、设备和普通文件，针对每个进程，文件描述符都有一套

```
文件描述符    用途    POSIX名称      stdio流
0         标准输入  STDIN_FILENO    stdin
1         标准输出  STDOUT_FILENO   stdout
2         标准错误  STDERR_FILENO   stderr
```



