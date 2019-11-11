### 第11章 系统限制和选项

##### 系统限制

针对每个限制，所有实现必须支持一个最小值，定义在<limits.h>中，常量名字为_POSIX_XXX_MAX，每个限制都有一个名称，与其常量名称对应，少了前面的 _POSIX

1. 运行时恒定值：固定不变的值，如MQ_PRIO_MAX，可用sysconf获取
2. 路径名变量值：与路径名相关的限制，每个系统可能不同，如NAME_MAX，可用pathconf或fpathconf获取
3. 运行时可增加值：相对于特定实现其值固定，但在运行时可能会增加，如NGROUPS_MAX，可用sysconf获取

在shell中可用使用getconf命令获取已然实现的限制和选项：

```
getconf ARG_MAX
262144
getconf NAME_MAX /bin/
255
```

##### 在运行时获取系统限制和选项

```
#include <unistd.h>

long sysconf(int name);
// 返回值：若成功，返回name所对应的限制值，若无法确定某限制或发生其他错误，返回-1
// name定义在unistd.h中的常量 _SC_XXX
```

根据要求，针对特定限制，调用sysconf获取的值在进程的生命周期内应保持不变；进程可以通过setrlimit修改进程的各种资源限制，这会影响到由sysconf返回的限制值

##### 在运行时获取与文件相关的限制和选项

```
#include <unistd.h>

long pathconf(const char *pathname, int name);
long fpathconf(int fd, int name);
// 两个函数的返回值：若成功，返回name所对应的限制值，若无法确定某限制或发生其他错误，返回-1
```

与sysconf不同，并不要求这两个函数的返回值在进程的生命周期内保持恒定，这是因为，在进程的运行期间，可能会卸载再重新加载文件系统

##### 不确定的限制

有时候，系统实现没有将一些系统限制定义为常量如PATH_MAX，并且sysconf或pathconf在返回时如 _PC_PATH_MAX 会将其归为不确定，对此，可以采用如下策略：

* 某些情况下，切实可行的方法是省去对限制的检查，用相关的系统调用或库函数代替
* 自行编写程序或函数，以估算或推断
* 编写可移植性程序时，选择所规定的最低限制值如 _POSIX_PATH_MAX，但有时不可行
* 利用诸如GNU Autoconf之类的扩展工具，它能确定各种系统的限制是否存在，如何设置

##### 系统选项

支持的选项主要包括：实时信号、POSIX共享内存、POSIX线程、任务控制之类的功能，在unistd.h中定义，这些常量前面都以   _POSIX_   或 _XOPEN_  为前缀，其值必为如下之一：

* -1：表示实现不支持
* 0：表示实现可能支持，需要sysconf、pathconf或fpathconf进行运行时检查
* 大于0：表示实现支持