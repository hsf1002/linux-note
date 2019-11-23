### 第16章 扩展属性

##### 概述

扩展属性EA，以名称-值的形式将任意元数据与文件i节点关联起来的技术，只能对普通文件和目录起效

命名空间格式为namespace.name，namespace的取值为user、trusted、system以及security：

* user：将在文件权限检查的制约下由非特权进程操控，mount时带user_xattr选项
* trusted：只有特权进程才能操控
* system：供内核使用，目前仅支持访问控制列表
* security：安全

一个i节点可以拥有多个相关EA，所属命名空间可以相同，也可以不同

```
// 添加EA
sky@sky-VirtualBox:~$ setfattr -n user.x -v 'the past is past' tfile 
sky@sky-VirtualBox:~$ setfattr -n user.y -v 'in fact, the past is not past' tfile 
// 查看某个EA
sky@sky-VirtualBox:~$ getfattr -n user.x tfile 
# file: tfile
user.x="the past is past"
// 查看所有EA
sky@sky-VirtualBox:~$ getfattr -d tfile 
# file: tfile
user.x="the past is past"
user.y="in fact, the past is not past"
// 将user.x设置为空
sky@sky-VirtualBox:~$ setfattr -n user.x tfile 
sky@sky-VirtualBox:~$ getfattr -d tfile 
# file: tfile
user.x
user.y="in fact, the past is not past"
// 将user.y删除
sky@sky-VirtualBox:~$ setfattr -x user.y tfile 
sky@sky-VirtualBox:~$ getfattr -d tfile 
# file: tfile
user.x
```

