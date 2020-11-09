# 附录C-对NULL指针做转型

一般来说，NULL会被定义为0或者(void *)0，，C预处理器会将NULL替换为0，C标准规定0可以用在任何需要用到指针的上下文中，大多数情况下不会有问题

```
int *p;

p = 0;     
p = NULL; // same as above

sigaction(SIGINT, &sa, 0);
sigaction(SIGINT, &sa, NULL);  // same as above
```

但是在类似execl这样的变参函数中，将NULL指针作为可变参数时，就必须做转型

```
execl("ls", "ls", "-l", (char *)NULL);
```

不做转型，程序在某些系统上会奔溃

* 编译器无法判断变参函数期望得到的可变参数的类型是什么
* C标准不要求NULL指针实际上以0来代替

因此下面的写法是错误的，因为无法保证在系统上0和NULL指针是等同的

```
execl(prog, arg, 0);
execl(prog, arg, NULL);
```

应该重写为如下形式

```
execl(prog, arg, (char *)0);
execl(prog, arg, (char *)NULL);
```

