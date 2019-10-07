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

为了使用一个共享库，需要做两件事情，而使用静态库无需：

* 链接阶段将共享库的名称嵌入到可执行文件中
* 运行时解析嵌入的库名，如果不在内存就要加载进来

动态链接器本身是一个共享库，是/lib/ld-linux.so.2，所有使用共享库的ELF可执行文件都要用到它

```
gcc -g -Wall -o prog prog.c libfoo.so
./prog
No such file or directory
```

出现上述错误是因为库位于当前目录，而不是动态链接器搜索的标准目录清单，可以使用LD_LIBRARY_PATH设置目录，动态链接器在搜索标准目录之前会先查找该目录

```
LD_LIBRARY_PATH=. ./prog
```

##### 共享库别名soname

引入soname的目的是为了提供一层间接，使得可执行程序能够在运行时与链接时使用的库不同的（兼容的）共享库

```
gcc -g -c -fPIC -Wall mod1.c mod2.c mod3.c
gcc -g -shared -Wl, -soname, libbar.so -o libfoo.so mod1.o mod2.o mod3.o
```

如果要确定一个既有共享库的soname，可以使用下面两个命令之一：

```
objdump -p libfoo.so | grep SONAME
readelf -d libfoo.so | grep SONAME
```

当使用soname时必须要创建一个符号链接，将soname指向库的真实名称，而且将其放入动态链接库搜索的目录

```
ln -s libfoo.so libbar.so
```

##### 共享库的有用工具

* ldd：显示一个程序运行时所需要的共享库
* objdump： 从可执行文件、目标文件、共享库获取包括反汇编的二进制机器码，还能显示这些文件各个ELF头部信息
* readelf：显示类似于objdump的信息，但格式不同
* nm：列出目标库或可执行文件中定义的一组符号，如果要找出哪个库定义了crypt()函数：

```
nm -A /usr/lib/lib*.so 2> /dev/null |grep 'crypt$'
```

-A选项指定了在显示符号的每一行开头列出库的名称，此处还丢弃了标准错误输出

##### 共享库的命名规则

* 真实名称：libname.so.major-id.minor-id
* soname：libname.so.major-id，一般情况下，每个库的主要版本的soname会指向主要版本最新的次要版本
* 链接器名称：libname.so，是不包含主要版本和次要版本的符号链接

链接器名称只存在一个实例，指向真实名称或者最新的soname符号链接；库的每个主要版本都存在一个soname，运行时用来找出指向相应的（最新的）真实名称的同名符号链接所引用的库

1. 创建目标文件

```
gcc -g -c -fPIC -Wall mod1.c mod2.c mod3.c
```

2. 创建共享库

```
gcc -g -shared -Wl, -soname, libdemo.so.1 -o libdemo.so.1.0.1 mod1.o mod2.o mod3.o
```

3. 创建符号链接

```
ln -s libdemo.so.1.0.1 libdemo.so.1
ln -s libdemo.so.1 libdemo.so
```

4. 使用链接器名称构建可执行文件

```
gcc -g -Wall -o prog prog.c -L. -ldemo
LD_LIBRARY_PATH=. ./prog
```

##### 安装共享库

标准目录包括：

* /usr/lib：大多数标准库的安装路径
* /lib：系统启动时用到的库安装到此路径
* /usr/local/lib：非标准或实验性的库
* /etc/ld.so.conf中列出的目录

大多数情况下，将文件复制到上述目录需要超级用户权限

```
mv libdemo.so.1.0.1 /usr/lib
cd /usr/lib
ln -s libdemo.so.1.0.1 libdemo.so.1
ln -s libdemo.so.1 libdemo.so
```

ldconfig命令可以解决两个问题：

* 如果动态链接器要搜索所有的目录找到并加装一个库，很慢
* 如果安装新版本或删除旧版本的库，soname符号链接就不是最新的

ldconfig命令可以执行两个任务：

* 搜索一组标准目录并创建或更新一个缓存文件/etc/ld.so.cache，使之包含每个库的主要版本的最新次要版本
* 检查每个库的主要版本的最新次要版本，找出嵌入的soname，在同一目录为每个soname创建或更新相对符号链接

为了正确执行这些动作，ldconfig要求库的名称要根据规范命名，默认情况下，会执行这两个任务

-N选项：防止缓存重建，-X选项会阻止soname符号链接的创建，-v选项输出执行动作的信息，-n选项创建soname的符号链接

安装一个新库、更新或删除一个旧库，/etc/ld.so.conf列表被修改后，都应该执行此命令

##### 升级共享库

如果要创建共享库/usr/lib/libdemo.so.1.0.1的一个新的次要版本

```
gcc -g -c -fPIC -Wall mod1.c mod2.c mod3.c
gcc -g -shared -Wl, -soname, libdemo.so.1 -o libdemo.so.1.0.2 mod1.o mod2.o mod3.o
mv libdemo.so.1.0.2 /usr/lib
ldconfig -v | grep libdemo
libdemo.so.1 -> libdemo.so.1.0.2
```

已经运行的程序要使用新的次要版本，只有当它们终止或重启后才会生效

如果要创建共享库/usr/lib/libdemo.so.1.0.1的一个新的次要版本

```
gcc -g -c -fPIC -Wall mod1.c mod2.c mod3.c
gcc -g -shared -Wl, -soname, libdemo.so.2 -o libdemo.so.2.0.0 mod1.o mod2.o mod3.o
mv libdemo.so.2.0.0 /usr/lib
ldconfig -v | grep libdemo
libdemo.so.1 -> libdemo.so.1.0.2
libdemo.so.2 -> libdemo.so.2.0.0
cd /usr/lib
ln -sf libdemo.so.2 libdemo.so
```

必须手动更新链接器名称的符号链接，最后一条命令