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

### 终端特殊字符

c_cc下标都是字符的常量名添加V，除了CR和NL没有对应的c_cc下标

```
字符     c_cc下标    描述    默认设定
CR        无        回车      ^M
DISCARD  VDISCARD  丢弃输出    ^O
EOF       VEOF     文件结尾    ^D
EOL                行结尾
EOL2              另一种行结尾
ERASE               擦除字符   ^?
INTR             中断(SIGINT) ^C
KILL                擦除一行   ^U
LNEXT   字面化下一个字符(一般是特殊字符) ^V
NL                   换行      ^J
QUIT             退出(SIGQUIT) ^\
REPRINT          重新打印输入行  ^R
START              开始输出     ^Q
STOP               停止输出     ^S
SUSP             暂停(SIGTSTP)  ^Z
WERASE           擦除一个字符    ^W
```

### 终端标志

```
c_iflag：
BRKINT：接到BREAK时产生SIGINT；
ICRNL：将输入的CR转换为NL；
IGNBRK：忽略BREAK条件；
IGNCR：忽略CR；
IGNPAR：忽略奇偶错字符；
IMAXBEL：在输入队列空时振铃；
INLCR：将输入的NL转换为CR；
INPCK：打开输入奇偶校验；
ISTRIP：剥除输入字符的第8位；
IUCLC：将输入的大写字符转换成小写字符(仅SVR4)；
IXANY：使任一字符都重新起动输出；
IXOFF：使起动/停止输入控制流起作用；
IXON：使起动/停止输出控制流起作用；
PARMRK：标记奇偶错；

c_oflag：
BSDLY：退格延迟屏蔽(仅SVR4)；
CRDLY：CR延迟屏蔽(仅SVR4)；
FFDLY：换页延迟屏蔽(仅SVR4)；
NLDLY：NL延迟屏蔽(仅SVR4)；
OCRNL：将输出的CR转换为NL(仅SVR4)；
OFDEL：填充符为DEL，否则为NUL(仅SVR4)；
OFILL：对于延迟使用填充符(仅SVR4)；
OLCUC：将输出的小写字符转换为大写字符(仅SVR4)；
ONLCR：将NL转换为CR-NL；
ONLRET：NL执行CR功能(仅SVR4)；
ONOCR：在0列不输出CR(仅SVR4)；
ONOEOT：在输出中删除EOT字符(仅4.3+BSD)；
OPOST：执行输出处理；
OXTABS：将制表符扩充为空格(仅4.3+BSD)；
TABDLY：水平制表符延迟屏蔽(仅SVR4)；
VTDLY：垂直制表符延迟屏蔽(仅SVR4)；

c_cflag：
CCTS_OFLOW：输出的CTS流控制(仅4.3+BSD)；
CIGNORE：忽略控制标志(仅4.3+BSD)；
CLOCAL：忽略解制解调器状态行；
CREAD：启用接收装置；
CRTS_IFLOW：输入的RTS流控制(仅4.3+BSD)；
CSIZE：字符大小屏蔽；
CSTOPB：送两个停止位，否则为1位；
HUPCL：最后关闭时断开；
MDMBUF：经载波的流控输出(仅4.3+BSD)；
PARENB：进行奇偶校；
PARODD：奇校，否则为偶校；

c_lflag：
ALTWERASE：使用替换WERASE算法(仅4.3+BSD)；
ECHO：进行回送；
ECHOCTL：回送控制字符为^(char)；
ECHOE：可见擦除符；
ECHOK：回送kill符；
ECHOKE：kill的可见擦除；
ECHONL：回送NL；
ECHOPRT：硬拷贝的可见擦除方式；
FLUSHO：刷清输出；
ICANON：规范输入；
IEXTEN：使扩充的输入字符处理起作用；
ISIG：使终端产生的信号起作用；
NOFLSH：在中断或退出键后不刷清；
NOKERNINFO：STATUS不使内核输出(仅4.3+BSD)；
PENDIN：重新打印；
TOSTOP：对于后台输出发送SIGTTOU；
XCASE：规范大/小写表示(仅SVR4)；
```

