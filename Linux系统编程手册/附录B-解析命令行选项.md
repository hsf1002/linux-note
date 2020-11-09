# 附录B-解析命令行选项

典型的UNIX命令行形式：

```
command [options] arguments
```

选择options的形式为-紧连着一个唯一的字符，以及针对该选项的可选参数，下面的命令等同：

```
grep -l -i -f patterns *.c
grep -lif patterns *.c
grep -lifpatters *.c

-l和-i没有参数，-f选项将字符串patterns作为参数
```

```
#include <unistd.h>

extern int optind, opterr, optopt;
extern char *optarg;

int getopt(int argc, char *const argv[], const char *optstring);
// 返回值：
每次调用都会返回下一个未处理选项的信息，如果找到选项，则返回该选项的字符，如果到达选项列表尾，返回-1，如果选项带有参数，则把全局变量optarg指向这个参数
```

optstring指定了函数getopt应该寻找的命令行选项集合，由一组字符组成，每个字符标识一个选项，每个选项后面可以跟一个冒号：，标识这个选项带有一个参数

通过连续调用getopt来解析命令行，每次调用getopt时，optind都会更新，指的是参数列表argv未处理的下一个元素的索引，首次调用getopt之前，optind自动设置为1，以下两种情况需要用到这个变量

* getopt返回-1，表示没有更多选项可解析，且optind的值比argc要小，argv[optind]表示命令行中下一个非选项单词
* 如果处理多个命令行向量或重新扫描相同的命令行，必须手动将optind设置为1

以下情况getopt返回-1

* 选项列表解析完毕，`argv[optind] == NULL`
* argv中下一个未处理的单字只由一个单独的连字符打头，`argv[optind][0]`不是连字符-
* argv中下一个未处理的单字由两个连字符--组成

```
// "a:b:cd::e"，这就是一个选项字符串。对应到命令行就是-a ,-b ,-c ,-d, -e 冒号表示参数
一个冒号就表示这个选项后面必须带有参数>    （没有带参数会报错哦），但是这个参数可以和选项连在一起写，也可以用空格隔开，比如-a123 和-a   123（中间有空格） 都表示123是-a的参数；
两个冒号的就表示这个选项的参数是可选的，即可以有参数，也可以没有参数，但要注意有参数时，参数与选项之间不能有空格（有空格会报错的哦）
```

