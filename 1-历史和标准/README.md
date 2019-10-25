### 第1章 历史和标准

##### UNIX和C语言简史

1969：AT&T的贝尔实验室，Ken Thompson使用Digital PDP-7小型机的汇编语言开发了首个UNIX实现

1970：AT&T的工程师们，使用Digital PDP-11以汇编语言重写了UNIX

1972：D.M.Ritchie 在B语言的基础上最终设计出了C语言，取了BCPL的第二个字母作为这种语言的名字

1973：C语言进入成熟期，使用它重写了几乎整个UNIX内核

1979：UNIX发布了第七个版本，从此分裂成了两大分支

* BSD：Berkeley Software Distribution
* System V：AT&T的UNIX团队支持，1989年发布System V Release 4，SVR4

##### Linux简史

1984：Richard Smallman创建了自由的UNIX实现，发起GNU（GNU's not UNIX）项目

1985：Richard Smallman创建了自由软件基金会（FSF），指定了GNU GPL（通用公共许可协议）

90年代早期：GNU已经开发出了GCC、Emacs、bash shell、glibc，几乎是一套完整的操作系统，却缺少UNIX内核

1991：Linux Torvalds为自己的Intel80386PC开发了一个操作系统，开始Linux的使用许可协议非常严格，但很快归于GNU GPL的阵营

2003：发布Linux内核2.6

##### 标准化

1989：ASNI（美国国家标准化委员会）的C语言标准在1990被ISO（国际标准委员会）所采纳，史称C89

1999：ISO正式批准了C语言标准的修订版，增加了long long、布尔类型、C++注释风格等，史称C99

2011：ISO发布的最新标准，包括字节对齐说明符、泛型机制、多线程的支持、静态断言、原子操作以及对 Unicode 的支持，史称C11

1989：POSIX.1（可移植操作系统接口）成功IEEE标准，稍作修改1990年被ISO采纳

1992：POSIX.2标准化

将XSI规范符合度达标所需的额外接口和行为统称为XSI扩展，支持以下特性：线程、mmap和munmap、dlopen API、资源限制、伪终端、System V IPC、syslog API、poll以及登录记账