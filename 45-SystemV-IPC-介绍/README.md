## 第45章 System V IPC介绍

### 概述

SUSv3因为遵循XSI要求实现System V IPC，因此也称为XSI IPC，通过CONFIG_SYSVIPC选项进行配置

![WechatIMG43.jpeg](https://i.loli.net/2020/01/30/IGkHQOD8AiUo9db.jpg)

##### 创建和打开一个IPC对象

msgget、semget、shmget，将完成下面的一个操作：

* 使用key创建一个新的对象，返回唯一的标识符
* 返回key对应的对象的标识符

一个进程可以通过指定IPC_EXCL标记确保它是创建IPC对象的进程

##### IPC对象的删除与持久

IPC_RMID适用于三种IPC类型，用来删除一个对象，对于信号量和消息队列而言，删除立即生效，对象包含的所有信息被销毁，不管是否有其他进程仍然在使用对象，共享内存对象的删除稍有不同，只有当使用该内存段的进程与该内存段分离之后才会删除（与文件的删除类似）。IPC对象具有持久性，一旦被创建，就一直存在直到被显式删除或系统关闭

