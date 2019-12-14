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

##### 为阻塞操作设置超时

定时器的用途之一，为某个阻塞系统调用设置时间上限，具体步骤如下：

1. 调用sigaction为SIGALRM创建处理函数，排除SA_RESTART标志以确保系统调用不会重新启动
2. 调用alarm或setitimer创建定时器
3. 执行阻塞系统调用
4. 系统调用返回后，再次调用alarm或setitimer屏蔽定时器
5. 检查系统调用失败时是否将errno置为了EINTR（系统调用遭到中断）

##### 暂停运行（休眠）一段时间

低分辨率休眠：

```
#include <unistd.h>

unsigned int sleep(unsigned int seconds);
// 返回值：若成功，返回0，若因信号中断，返回剩余秒数
// Linux将sleep实现为对nanosleep的调用
```

高分辨率休眠：

```
#define _POSIX_C_SOURCE 199309
#include <time.h>

int nanosleep(const struct timespec *request, struct timespec *remain);
// 返回值：若成功，返回0，若出错或被中断，返回-1
// 若参数remain不是NULL，则其返回剩余的休眠时间，可利用该值在系统调用重启后完成休眠
// SUSv3明确规定不得使用信号实现该函数，意味着可以与alarm和setitimer混用，而不会危及移植性
// 可以通过信号处理函数将其中断，此时返回-1，errno置为EINTR

struct timespec
{
    time_t tv_sec;
    long tv_nsec;  // 纳秒级别
}
```

##### POSIX时钟

Linux中，调用此API需要以-lrt选项进行编译，从而与librt函数库链接

获取时钟的值：

```
#define _POSIX_C_SOURCE 199309
#include <time.h>

int clock_gettime(clcokid_t clockid, struct timespec *tp);
int clock_getres(clockid_t clockid, struct timespec *res);
// 返回值：若成功，返回0，若出错，返回-1
// clockid的类型：
CLOCK_REALTIME：可设定的系统级实时时钟
CLOCK_MONOTONIC：不可设定的恒定态时钟（适用于无法忍受系统时钟发生跳跃性变化）
CLOCK_PROCESS_CPUTIME_ID：进程CPU时间的时钟
CLOCK_THREAD_CPUTIME_ID：线程CPU时间的时钟
```

设置时钟的值：

```
int clock_settime(clcokid_t clockid, const struct timespec *tp);
// 返回值：若成功，返回0，若出错，返回-1
// 特权级进程可设置CLOCK_REALTIME类型时钟
```

获取特定进程的时钟ID：

```
#define _XOPEN_SOURCE 600
#include <time.h>

int clock_getcpuclockid(pid_t pid, clockid_t *clockid);
// 返回值：若成功，返回0，若出错，返回负值
// pid为0时，返回调用进程的CPU时间时钟ID
```

获取特定线程的时钟ID：

```
#define _XOPEN_SOURCE 600
#include <time.h>
#include <pthread.h>

int pthread_getcpuclockid(pthread_t tid, clockid_t *clockid);
// 返回值：若成功，返回0，若出错，返回负值
```

高分辨率休眠的改进版：clock_nanosleep

```
#define _XOPEN_SOURCE 600
#include <time.h>

int clock_nanosleep(clcokid_t clockid, int flags, const struct timespec *request, struct timespec *remain);
// 返回值：若成功，返回0，若出错，返回负值
```

##### POSIX间隔定时器

使用setitimer来设置经典UNIX间隔定时器，有些制约：

* 类型只能是ITIMER_REAL、ITIMER_VIRTUAL、ITIMER_PROF的一个
* 只能通过发送信号方式通知定时器到期，也不能改变到期时产生的信号
* 如果一个间隔定时器到期多次，且相应信号遭到阻塞，只会调用一次信号处理函数，无法确定定时器溢出情况
* 定时器分辨率只能到达微秒级

为了突破这些限制，可使用Linux定义的实现了POSIX.1b的一套API，需要编译时添加-lrt选项，由fork创建的子进程不会继承POSIX定时器，调用exec期间或进程终止时将停止并删除定时器

创建定时器：

```
#define _POSIX_C_SOURCE 199309
#include <time.h>
#include <signal.h>

int timer_create(clockid_t clockid, struct sigevent *evp, timer_t *timerid);
// 返回值：若成功，返回0，若出错，返回-1
// clockid是CLOCK_REALTIME、CLOCK_MONOTONIC、CLOCK_PROCESS_CPUTIME_ID、CLOCK_THREAD_CPUTIME_ID、CLOCK_BOOTTIME、CLOCK_REALTIME_ALARM、CLOCK_BOOTTIME_ALARM的一种
// 返回时将定时器句柄保存在timerid，供后续调用
// 参数evp决定到期时程序的通知方式，若是NULL，表明将sigev_notify置为SIGEV_SIGNAL，同时将sigev_signo置为SIGALRM，并将sigev_value.sival_int置为定时器ID

union sigval 
{          
 	  int  sival_int;         /* 通知时的伴随数据：Integer value */
    void  *sival_ptr;       /* 通知时的伴随数据：Pointer value */
};

struct sigevent 
{
	  int sigev_notify; /* 通知方式 */
    int sigev_signo;  /* 通知信号 */
    union sigval sigev_value;  /* 通知时的伴随数据 */
    
    void (*sigev_notify_function) (union sigval);/* SIGEV_THREAD通知时调用的函数 */
    void *sigev_notify_attributes;/* 线程属性 */
    pid_t sigev_notify_thread_id; /* 线程ID，可用gettid或clone的返回值赋值 */
};

SIGEV_NONE：空的提醒，事件发生时不做任何事情
SIGEV_SIGNAL：向进程发送sigev_signo中指定的信号
SIGEV_THREAD：通知进程在一个新的线程中启动sigev_notify_function函数，函数的实参是sigev_value
SIGEV_THREAD_ID：发送sigev_signo信号给sigev_notify_thread_id标识的线程
```

