### 第23章 定时器与休眠

##### 间隔定时器

```
#include <sys/time.h>

int setitimer(int which, const struct itimerval *new_value, struct itimerval *old_value);
// 返回值：若成功，返回0，若出错，返回-1
// 如果不关心定时器的前一设置，可以置old_value为NULL
// which的取值：
ITIMER_REAL：以系统真实的时间来计算，它送出SIGALRM信号
ITIMER_VIRTUAL：以该进程在用户态下花费的时间来计算，它送出SIGVTALRM信号
ITIMER_PROF：以该进程在用户态下和内核态下所费的时间来计算，它送出SIGPROF信号
// 对所有信号的默认处置是终止进程，除非希望如此，否则应该创建对应的信号处理函数

struct itimerval 
{
    struct timeval it_interval; // 下一次延迟的时间，如果为0表示一次性定时器
    struct timeval it_value;    // 第一次延迟的时间
};

struct timeval {
    time_t      tv_sec;         /* seconds */
    suseconds_t tv_usec;        /* microseconds */
};
```

可以在任意时刻调用getitimer，了解定时器的当前状态，距离下次到期的剩余时间：

```
int getitimer(int which, struct itimerval *curr_value);
// 返回值：若成功，返回0，若出错，返回-1
// curr_value.it_value返回距离下一次到期所剩余的时间
```

使用setitimer和alarm创建的定时器可以跨越exec调用得以保存，但由fork创建的子进程并不继承；SUSv4已经废止了getitimer和setitimer，同时推荐POSIX定时器API

##### 一次性定时器

系统调用alarm为创建一次性定时器提供了简单接口

```
#include <unistd.h>

unsigned int alarm(unsigned int seconds);
// 总是返回成功，若之前设置了定时器返回剩余时间，若没有设置返回0
// seconds表示定时器到期的秒数，到时会发送SIGALRM信号
// 调用alarm()会覆盖定时器的前一个设置，调用alarm(0)可屏蔽当前定时器
// 如果使用alarm()设置定时器且不设置信号处理函数，可以通过此技术确保杀死进程
// Linux中，setitimver和alarm针对同一进程共享同一实时定时器
```

##### 定时器的调度与精度

传统意义上的定时器精度受制于软件时钟频率，如19100微秒即19毫秒的定时器，如果jiffy（软件时钟周期）为4毫秒，实际定时器每隔20毫秒才会过期，可通过内核配置CONFIG_HIGH_RES_TIMERS选择支持高分辨定时器，现代硬件平台，精度达到纳秒级别则已是司空见惯了

