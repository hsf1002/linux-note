### 第41章 共享库基础

##### 目标库

编译：

```
cc -g -c prog.c mod1.c mod2.c mod3.c
```

链接：

```
cc -g -o prog_nolib prog.o mod1.o mod2.o mod3.o
```

-g选项表示包含调试信息，可执行文件和库不应该使用strip删除调试信息

gcc能够确保使用正确的选项来调用ld（链接器程序）并将程序与正确的库文件链接起来

共享库是一种比静态库更加现代化的对象库

