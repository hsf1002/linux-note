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

##### 共享库概述

将程序与静态库链接起来，得到的可执行文件会包含所有被链接的目标文件的副本，当几个程序使用同样的目标模块时，每个程序都有拥有自己的目标模块的副本，这有几个缺点：

* 浪费磁盘空间
* 如果几个程序使用的目标模块在同一时间运行，每个程序会独立的在虚拟内存中保存目标模块的副本
* 如果一个目标模块变更，所有使用它的可执行程序都必须重新链接

共享库就是设计解决这些问题，其关键思想是目标模块的单个副本由所有需要这个模块的程序共享，虽然代码共享，但是其中的变量依然每个程序拥有单独的副本，共享库还有以下优点：

* 整个程序的大小变得更小了
* 由于目标模块没有复制到可执行程序，而是在共享库集中维护，目标模块变更时，无需重新链接程序

共享库的主要开销：

* 比静态库更加复杂
* 编译时必须使用位置独立的代码
* 运行时必须要执行符号重定位

##### 创建和使用共享库

目前我们只关心ELF（Executable and Linking Format）共享库，现代版本的Linux和其他UNIX实现都采用了这种格式，取代了早期的a.out和COFF格式

创建一个共享库：

```
gcc -g -c -fPIC -Wall mod1.c mod2.c mod3.c
gcc -g -shared -o libfoo.so mod1.o mod2.o mod3.o
```

共享库的前缀是lib，后缀是.so，可以使用一个命令：

```
gcc -g -fPIC -Wall mod1.c mod2.c mod3.c -shared -o libfoo.so
```

与普通的可执行文件一样，共享库的目标文件不再维护不同的身份

-fPIC选项指定编译器应该生成位置独立的代码，这对于共享库来说是必须的，为了确定一个既有目标模块是否使用了该选项，可以使用下面两个命令之一查询：

```
nm mod1.o | grep _GLOBAL_OFFSET_TABLE_
readelf -s mod1.o | grep _GLOBAL_OFFSET_TABLE_
```

如果下面两个命令之一产生了任何输出，说明共享库中至少一个目标模块编译时没有指定-fPIC选项：

```
objdump --all-headers libfoo.so | grep TEXTREL
readelf -d libfoo.so | grep TEXTREL
```

TEXTREL表示存在一个目标模块，其文本段中包含需要运行时重定位的引用



