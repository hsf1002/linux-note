# 附录A-跟踪系统调用

strace命令允许我们跟踪程序执行的系统调用，对于调试程序或程序做些什么非常有帮助

```
strace command arg...

-o：默认其输出到stderr，-o filename 可以指定输出路径
-v：字段是结构体，则显示整个结构体
-s：字符串以文本显示，最长32字符，-s strsize 可以设置上限
-e：选择要跟踪的事件
-p：指定进程，非特权用户只能跟踪自己以及那些没有设置用户ID或组ID的程序
-c：打印出所有系统调用的概要，包括调用次数，失败次数，花费时间等
-f：使得子进程也能跟踪
```

strace的输出一般很长，因为包含了C运行时库启动代码以及家加装共享库时执行的系统调用，可进行过滤

```
strace date 2>&1 | grep open
// 跟踪open和close系统调用
strace -e trace=open, close date
```

