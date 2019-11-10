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



