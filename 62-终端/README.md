## 第62章 终端

### 概述

Linux上，/dev/ttyn设备是系统上虚拟控制台，tty是teletype的缩写

传统终端和终端模拟器（如伪终端）都需要同终端驱动程序关联，其负责处理设备上的输入和输出，执行输入时，驱动程序可以在如下两种模式：

* 规范模式：终端的输入按行处理，且可进行行编辑操作，这是默认的输入模式
* 非规范模式：终端输入不会装配为行，如vi、more等，无需按回车键就能读取到单个字符

驱动程序可以对特殊字符做出解释，如中断字符Ctrl+C、文件结尾Ctrl+D，非范围模式下无法处理某些或所有这些特殊字符

### 获取和修改终端属性

```
#include <termios.h>

int tcgetattr(int fd, struct termios *termios_p);
int tcsetattr(int fd, int opt_act, const struct termios *termios_p);
// 两个函数返回值：若成功，返回0，若出错，返回-1，若fd不指向终端，返回失败，错误码ENOTTY
// 如果任何一个属性的修改执行，则tcsetattr返回成功，只有所有属性修改请求都没有执行，才会返回失败

struct termios
{
    tcflag c_iflag; /* 输入模式标志*/
    tcflag c_oflag; /* 输出模式标志*/
    tcflag c_cflag; /* 控制模式标志*/
    tcflag c_lflag; /* 区域模式标志或本地模式标志或局部模式*/
    cc_t c_line;    /* 行规程，一直设置为新规程：N_TTY */
    cc_t c_cc[NCC]; /* 控制字符特性*/
    speed_t c_ispeed; /* unused */
    speed_t c_ospeed; /* unused */
};

opt_act的取值：
TCSANOW: 修改立即生效
TCSADRAIN: 所有当前处于排队的输出已经传送到终端后，修改生效
TCSAFLUSH: 效果同TCSADRAIN，但是除此之外，该标志生效时仍然等待处理的输入数据都会丢弃
```

### stty命令

以命令行的形式模拟函数tcgetattr和tcsetattr的功能

##### 检视终端当前所有属性

```
stty -a
// 线速，终端大小
speed 9600 baud; 36 rows; 128 columns; 
// 标志前带连字符-表示当前被禁用
// 控制终端输入的用户界面标志
lflags: icanon isig iexten echo echoe -echok echoke -echonl echoctl
	-echoprt -altwerase -noflsh -tostop -flusho pendin -nokerninfo
	-extproc
// 控制终端输入的标志	
iflags: -istrip icrnl -inlcr -igncr ixon -ixoff ixany imaxbel iutf8
	-ignbrk brkint -inpck -ignpar -parmrk
// 控制终端输出的标志
oflags: opost onlcr -oxtabs -onocr -onlret
// 控制终端线速的硬件控制相关标志
cflags: cread cs8 -parenb -parodd hupcl -clocal -cstopb -crtscts -dsrflow
	-dtrflow -mdmbuf
// 特殊字符	
cchars: discard = ^O; dsusp = ^Y; eof = ^D; eol = <undef>;
	eol2 = <undef>; erase = ^?; intr = ^C; kill = ^U; lnext = ^V;
	min = 1; quit = ^\; reprint = ^R; start = ^Q; status = ^T;
	stop = ^S; susp = ^Z; time = 0; werase = ^W;
```

##### 修改中断字符为Ctrl + L

```
skydeiMac:62-终端 sky$ sleep 30
^C
skydeiMac:62-终端 sky$ stty intr ^L
skydeiMac:62-终端 sky$ sleep 30
^C^C^C^L
skydeiMac:62-终端 sky$ stty -a |grep intr
	eol2 = <undef>; erase = ^?; intr = ^L; kill = ^U; lnext = ^V;
```

##### 修改中断字符为非控制字符如q

```
skydeiMac:62-终端 sky$ stty intr q
skydeiMac:62-终端 sky$ sleep 30
^C^C^C^Lq
skydeiMac:62-终端 sky$ stty -a |grep intr
	eol2 = <undef>; erase = ^?; intr = q; kill = ^U; lnext = ^V;
```

##### 启动/关闭某个终端标志

```
skydeiMac:62-终端 sky$ stty -a |grep tostop
	-echoprt -altwerase -noflsh -tostop -flusho pendin -nokerninfo
	
skydeiMac:62-终端 sky$ stty tostop
skydeiMac:62-终端 sky$ stty -a |grep tostop
	-echoprt -altwerase -noflsh tostop -flusho pendin -nokerninfo
	
skydeiMac:62-终端 sky$ stty -tostop
skydeiMac:62-终端 sky$ stty -a |grep tostop
	-echoprt -altwerase -noflsh -tostop -flusho pendin -nokerninfo	
```

##### 终端处于可以显示但不可用状态时

当程序奔溃时，除了奢侈的关闭再重新打开一个窗口外，可以输入如下指令，将终端标志和特殊字符还原到一个合理的状态：

```
// Ctrl + J才是真正的换行符，某些模式下可能换行符不再映射为Enter键
Ctrl + J stty sane Ctrl + J 
```

##### 监视stty命令的终端属性

```
su
password: 

stty -a -F /dev/ttyt3
```

