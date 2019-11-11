### 第12章 系统和进程信息

##### /proc文件系统

/proc虚拟文件系统驻留在/proc目录，包含了各种内核信息，允许进程通过常规文件IO系统调用方便的读取，有时还可以修改，之所以是虚拟，因为其包含的文件和子目录并未存储在磁盘上，而是由内核在进程访问此类信息时动态创建的

对于每个进程，都有相应的目录/proc/PID，PID是进程的ID，该目录包含了如下进程的相关信息：

```
cmdline: 以\0分割的命令行参数
cwd: 指向当前工作目录的符号链接
Environ: NAME=value的环境列表，以\0分割
exe: 指向正在执行文件的符号链接
fd: 文件目录，包含指向由进程打开文件的符号链接（符号链接/proc/self表示当前进程自己的/proc/PID目录）
maps: 内存映射
mem: 进程虚拟内存
mounts: 进程的安装点
root: 指向根目录的符号链接
status: 各种信息
task: 为进程中每个线程包含一个子目录（/proc/PID/task/TID子目录表示进程PID下线程ID为TID的子目录）
```

/proc目录下的系统信息：

```
/proc: 各种系统信息
/proc/net: 网络和套接字的状态信息
/proc/sys/fs: 文件系统相关设置
/proc/sys/vm: 内存管理设置
/proc/sys/net: 网络和套接字的设置
/proc/sys/kernel: 常规的内核设置
/proc/sysvipc: System V IPC对象的信息
```

##### 系统表示：uname

```
#include <sys/utsname.h>

int uname(struct utname *utsbuf);
// 返回值：若成功，返回0，若出错，返回-1

struct utsname
{ 
	 char sysname[_UTSNAME_LENGTH];//当前操作系统名，内核自动设置
   char nodename[_UTSNAME_LENGTH];//网络上的名称，由sethostname设置
   char release[_UTSNAME_LENGTH];//当前发布级别，内核自动设置
   char version[_UTSNAME_LENGTH];//当前发布版本，内核自动设置
   char machine[_UTSNAME_LENGTH];//当前硬件体系类型，内核自动设置
#ifdef _GNU_SOURCE
	 char domainname[_UTSNAME_LENGTH];//当前域名，由setdomainname设置
#endif
};
```