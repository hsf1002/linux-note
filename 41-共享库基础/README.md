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

直接编译链接为一个可执行文件：

```
cc prog.c -o prog
如果要加载库：
cc -g -Wall -o prog prog.c libgetnum.so
```

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

##### 在目标文件中指定库搜索目录

通知动态链接库共享库目录的方式：

* 使用环境变量LD_LIBRARY_PATH
* 将共享库安装到标准目录
* 使用-rpath选项在静态编译阶段在可执行文件中插入一个在运行时搜索共享库的目录列表

-rpath选项的一个替代方案是环境变量LD_RUN_PATH，程序运行时会按照rpath指定的目录列表来搜索

假如程序prog依赖共享库libx1.so，而libx1.so又依赖于libx2.so

1. 构建libx2.so

```
cd /home/dir/d2
gcc -g -c -fPIC -Wall modex2.c
gcc -g -shared -o libx2.so modx2.o
```

2. 构建libx1.so

```
cd /home/dir/d1
gcc -g -c -fPIC -Wall modex1.c
gcc -g -shared -o libx1.so modx1.o -Wl, -rpath, /home/dir/d2 -L/home/dir/d2 -lx2
```

3. 构建主程序

```
cd /home/dir
gcc -g -Wall -o prog prog.c -Wl, -rpath, /home/dir/d1 -L/home/dir/d1 -lx1
```

无需指定libx2.so，链接器能够分析libx1.so的rpath列表，能够找到libx2.so，同时在静态链接阶段解析出所有符号

使用下面命令可以查看prog和libx1.so的rpath列表：

```
objdump -p prog | grep PATH
objdump -p d1/libx1.so | grep PATH
```

使用ldd命令可以列出prog的完整的动态依赖列表：

```
ldd prog
```

`DT_RPATH和DT_RUNPATH`，两种rpath列表差别在于动态链接请在运行时搜索共享库时他们相对于LD_LIBRARY_PATH的优先级，DT_RPATH的优先级比DT_RUNPATH较高；默认情况下，链接器会将rpath列表创建为DT_RPATH标签，为了创建DT_RUNPATH条目，必须使用--enable-new-dtags选项

```
gcc -g -Wall -o prog prog.c -Wl, --enable-new-dtags -Wl, -rpath, /home/dir/d1 -L/home/dir/d1 -lx1
objdump -p prog | grep PATH
RPATH    /home/dir/d1
RUNPATH  /home/dir/d1
```

为了使共享库位于包含应用程序的可执行文件的目录的子目录，即将共享库放在应用程序的子目录lib下：

```
gcc -Wl, -rpath, '$ORIGIN'/lib ...
```

##### 运行时找出共享库

动态链接器搜索共享库的规则：

1. 如果可执行文件的DT_RPATH列表包含目录且不包含DT_RUNPATH列表，搜索这些目录
2. 如果定义了LD_LIBRARY_PATH环境变量，搜索其指定的目录
3. 如果可执行文件的DT_RUNPATH列表包含目录，搜索这些目录
4. 检查/etc/ld.so.cache文件以确认它是否包含了与库相关的条目
5. 搜索/lib和/usr/lib目录

##### 运行时符号解析

如果主程序和共享库中同时定义了同名的全局符号（全局变量或全局函数），那么：

* 主程序的全局符号覆盖其他地方的定义
* 如果一个全局符号在多个库由定义，对该符号的引用会绑定到在扫描库时找到的第一个定义

如果要确保共享库中的定义为优先（即覆盖主程序的定义），构建共享库时需要使用-Bsymbolic选项

```
gcc -g -c -fPIC -Wall -c foo.c
gcc -g -shared -Wl, Bsymbolic -o libfoo.so foo.o
gcc -g -o prog prog.c libfoo.so
LD_LIBRARY_PATH=. ./prog
```

##### 使用静态库取代共享库

某些场景静态库更加适合，如用户不希望或者无法在运行程序的系统安装共享库或程序在另一个无法使用共享库的环境中运行；默认情况下，如果存在同名的静态库和共享库，优先使用共享库，如果要强制使用静态库，则使用如下操作之一：

* 在gcc命令行中指定静态库的路径名
* 在gcc命令行中指定-static选项
* 使用-Wl, -Bstatic和-Wl, -Bdynamic 选项显式的指定链接器选择静态库还是共享库

##### gcc编译选项

一般选项 Overall Option

| 选项 | 作用                                        |
| ---- | ------------------------------------------- |
| -v   | 打印命令到stderr                            |
| -E   | 预处理                                      |
| -c   | 编译、汇编到目标代码，不进行链接            |
| -o   | 输出到指定文件。如果没有指定，则输出到a.out |

语言选项 Language Option

| 选项  | 作用                                                |
| ----- | --------------------------------------------------- |
| -std= | 编译时遵循的语言标准，目前支持C/C++，如c99, c++0x等 |

目录选项 Directory Option

| 选项  | 作用                              |
| ----- | --------------------------------- |
| -ldir | 把dir加入到搜索头文件的路径列表中 |
| -Ldir | 把dir加入到搜索库文件的路径列表中 |

预编译选项 Preprocessor Option

| 选项              | 作用                                                      |
| ----------------- | --------------------------------------------------------- |
| -Dname=definition | 定义预编译宏，名字name，值definition                      |
| -Dname            | 定义预编译宏，名字name，值为1                             |
| -M                | 告诉预处理器输出一个make rule，描述源代码文件依赖哪些文件 |

链接选项 Linker Option

| 选项      | 作用                                                         |
| --------- | ------------------------------------------------------------ |
| -lx       | 进行链接时搜索名为libx.so的库                                |
| -shared   | 生成动态库                                                   |
| -static   | 生成静态库                                                   |
| -rdynamic | 链接器将所有符号添加到动态符号表中，方便dlopen()等使用       |
| -s        | 去除可执行文件中的符号表和重定位信息。用于减小可执行文件的大小 |

代码生成选项 Code Generation Option

| 选项                           | 作用                                                         |
| ------------------------------ | ------------------------------------------------------------ |
| -fPIC                          | 编译动态库时，要求产生与位置无关代码(Position-Independent Code) |
| -fvisibility=default \| hidden | 默认情况下，设置ELF镜像中符号的可见性为public或hidden        |

警告选项 Warning Option

| 选项      | 作用                                 |
| --------- | ------------------------------------ |
| -Wall     | 允许发出gcc提供的所有有用的报警信息  |
| -Wextra   | 对所有合法但值得怀疑的表达式发出警告 |
| -Werror   | 把告警信息当做错误信息对待           |
| -pedantic | 允许发出ANSI C标准所列的全部警告信息 |

调试选项 Debugging Option

| 选项      | 作用                                          |
| --------- | --------------------------------------------- |
| -g        | 产生带有调试信息的目标代码                    |
| -ggdb     | 生成gdb专 用的调试信息，会有一些gdb专用的扩展 |
| -gdwarf-2 | 产生DWARF version2 的格式的调试信息           |

优化选项 Optimization Option

| 选项                    | 作用                                                         |
| ----------------------- | ------------------------------------------------------------ |
| -O0                     | 不优化，是缺省值                                             |
| -O1                     | 尝试优化编译时间和可执行文件大小                             |
| -O2                     | 尝试几乎全部的优化功能，但不会进行“空间换时间”的优化方法。   |
| -O3                     | 再打开一些优化选项：-finline-functions， -funswitch-loops 和 -fgcse-after-reload |
| -O                      | 等同与-O1                                                    |
| -Os                     | 对生成文件大小进行优化。打开 -O2 开的全部选项，除了会那些增加文件大小的 |
| -fomit-frame-pointer    | 去掉所有函数SFP（Stack Frame Pointer），即在函数调用时不保存栈帧指针SFP。可以提高程序运行速度， 代价是不能通过backtrace进行调试 |
| -fno-omit-frame-pointer | 与-fno-omit-frame-poi                                        |

平台相关选项

| 选项  | 作用                                                  |
| ----- | ----------------------------------------------------- |
| -m32  | int、long和指针是32位，产生代码在i386系统上运行       |
| -m64  | int为32位、long和指针是64位，产生代码x86-64架构上运行 |
| -mx32 | int、long和指针是32位，产生代码x86-64架构上运行       |

