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
// 返回值：若成功，返回对应的限制值，若无法确定某限制或发生其他错误，返回-1
// name定义在unistd.h中的常量 _SC_XXX
```

根据要求，针对特定限制，调用sysconf获取的值在进程的生命周期内应保持不变；进程可以通过setrlimit修改进程的各种资源限制，这会影响到由sysconf返回的限制值

