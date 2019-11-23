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

##### 扩展属性的系统调用

创建和修改：

```
#include <sys/types.h>
#include <sys/xattr.h>

int setxattr(const char *path, const char *name, const void *value, size_t size, int flags);
int lsetxattr(const char *path, const char *name, const void *value, size_t size, int flags);
int fsetxattr(int fd, const char *name, const void *value, size_t size, int flags);
// 三个函数的返回值：若成功，返回0，若出错，返回-1
// flag的取值：
XATTR_CREATE: 若给定的name已经存在，则失败
XATTR_REPLACE: 若给定的name不存在，则失败

char *value = "the past is not dead";

if (-1 == setxattr(pathname, "user.x", value, strlen(value), 0))
	perror("setxattr error");
```

获取：

```
#include <sys/types.h>
#include <sys/xattr.h>

ssize_t getxattr(const char *path, const char *name, void *value, size_t size);
ssize_t lgetxattr(const char *path, const char *name, void *value, size_t size);
ssize_t fgetxattr(int fd, const char *name, void *value, size_t size);
// 三个函数的返回值：若成功，返回EA个数，若出错，返回-1
```

删除：

```
#include <sys/types.h>
#include <sys/xattr.h>

int removexattr(const char *path, const char *name);
int lremovexattr(const char *path, const char *name);
int fremovexattr(int fd, const char *name);
// 三个函数的返回值：若成功，返回0，若出错，返回-1
```

获取与文件相关的所有EA名称：

```
#include <sys/types.h>
#include <sys/xattr.h>

ssize_t listxattr(const char *path, char *list, size_t size);
ssize_t llistxattr(const char *path, char *list, size_t size);
ssize_t flistxattr(int fd, char *list, size_t size);
// 三个函数的返回值：若成功，返回复制到list中的字节数，若出错，返回-1
// 出于安全考虑，list中返回的EA可能不包含进程无权访问的属性名
```

