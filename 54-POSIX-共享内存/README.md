## 第54章 POSIX共享内存

### 概述

POSIX共享内存能够让无关进程共享一个映射区域而无需创建一个相应的映射文件，Linux使用挂载于/dev/shm下的tmpfs文件系统，POSIX共享内存的shm_open和mmap的关系类似于System V共享内存的shmget和shmat的关系，POSIX共享内存对象的引用通过文件描述符完成，因此可直接使用UNIX各种文件描述符相关的系统调用

