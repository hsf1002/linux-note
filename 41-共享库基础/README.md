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

##### 静态库

也被称为归档文件，是UNIX系统提供的第一种库，实际上是一个保存所有被添加到其中的目标文件的副本的文件，还记录着每个目标文件的各种特性，包括文件权限、数字用户和组ID和最后修改时间，根据惯例，其名称为libname.a，使用ar命令创建和维护静态库

* r（替换）

```
cc -g -c mod1.c mod2.c mod3.c
ar r libdemo.a mod1.o mod2.o mod3.o
rm mod1.o mod2.o mod3.o
```

* t（目录表）

```
ar tv libdemo.a
v可以看到其他所有属性
rw-r--r--     501/20           96 Oct  7 07:13 2019 __.SYMDEF SORTED
rw-r--r--     501/20         7240 Oct  7 07:12 2019 5901.o
```

* d（删除）

```
ar d libdemo.a mod3.o
```

使用静态库，两种方式：

1. 链接命令中指定静态库的名称

```
cc -g -c prog.c
cc -g -o prog prog.o libdemo.a
```

2. 将静态库放在一个链接器搜索的标准目录（如/usr/lib），然后使用-l 选项指定库名

```
cc -g -o prog prog.o -ldemo
```

如果不在链接器搜索目录，可以用-L 选项指定搜索目录

```
cc -g -o prog prog.o -Lmylibdir -ldemo
```



