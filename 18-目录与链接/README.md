### 第18章 目录与链接

##### 目录与（硬）链接

目录的存储方式与文件类似，区别有二：

* 在i-node条目中，将目录标记为不同的文件类型
* 目录是经过特殊组织而成的文件，本质上是一个表格，包含文件名和i-node编号

![WechatIMG21.jpeg](https://i.loli.net/2019/11/24/giA4Xk1nSDKUEax.jpg)

i-node编号始于1用来记录文件系统中的坏块，0表示该条目未使用，文件系统的根目录总是存储在i-node条目2中，i-node列表中没有文件名，这样在相同或不同的目录中可以创建多个名称，每个都指向相同的i-node节点，这些名称就是硬链接

```
echo -n 'it is good to collect things,' > abc
ln abc xyz
echo 'but it is better to go on walks.' >> xyz
cat abc
it is good to collect things,but it is better to go on walks.
ls -li abc xyz   // 第三列表示对i-node链接的计数
7284800 -rw-r--r--  2 sky  staff  62 11 24 21:25 abc
7284800 -rw-r--r--  2 sky  staff  62 11 24 21:25 xyz
```

```
rm abc
ls -li abc xyz
ls -li xyz
7284800 -rw-r--r--  1 sky  staff  62 11 24 21:25 xyz
```

仅当i-node链接计数为0时，即移除了文件的所有名字时，才会删除文件的i-node记录和数据块；一个文件描述符指向一个i-node，而这个i-node的文件名则可能有多个甚至一个都没有，所以无法通过文件描述符确定文件名，Linux上可以通过/proc/PID/fd目录内容的扫描，获知一个进程当前打开了哪些文件

对硬链接的限制有二：

1. 硬链接必须与指代的文件驻留在同一文件系统
2. 不能为目录创建硬链接，否则会出现链接环路

##### 符号链接

也称软链接，是一种特殊的文件类型，其数据是另一个文件的名称，因为符号链接指代一个文件，而非i-node编号，所以可以用来链接不同文件系统的一个文件，也可以为目录创建符号链接，符号链接之间可能形成链路，Linux会将对一个完整路径名的解引用总数限制为40次，意在应对超长符号链接链路和环路

![WechatIMG22.jpeg](https://i.loli.net/2019/11/24/jBuU7qh5bpwft4m.jpg)

是否对系统调用的路径名进行解引用，有一点是约定俗成，总是会对路径中的目录部分的符号链接进行解引用，而是否对文件名本身进行解引用，取决于系统调用，大部分操作会无视符号链接的所有权和权限，是否允许操作是由其所指代的文件的所有权和权限决定，仅当带有sticky权限位的目录对符号链接进行移除或改名时，才会考虑符号链接自身的所有权



