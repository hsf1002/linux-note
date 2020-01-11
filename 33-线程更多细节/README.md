## 线程：更多细节

### 线程限制

可以通过sysconf函数查看线程相关限制

```
PTHREAD_DESTRUCTOR_ITERATIONS: 销毁一个线程数据最大的尝试次数，通过_SC_THREAD_DESTRUCTOR_ITERATIONS查询
PTHREAD_KEYS_MAX: 一个进程可以创建的最大key的数量,通过_SC_THREAD_KEYS_MAX查询 
PTHREAD_STACK_MIN: 线程可以使用的最小的栈空间大小, 通过_SC_THREAD_STACK_MIN查询
PTHREAD_THREADS_MAX:一个进程可以创建的最大的线程数, 通过_SC_THREAD_THREADS_MAX查询
```

### 线程属性

```
int pthread_attr_init(pthread_attr_t *attr);   
int pthread_attr_destroy(pthread_attr_t *attr); 
// 两个函数返回值：若成功，返回0，若出错，返回错误编号
```

pthread_attr_init会对属性对象动态分配空间，而pthread_attr_destroy不仅会释放该空间，还会用无效值初始化属性对象，如果该属性对象被误用，导致pthread_create函数返回错误码

线程属性包括：

```
detachstate: 线程的分离状态属性
guardsize: 线程栈末尾的警戒缓冲区大小
statckaddr: 线程栈的最低地址
stacksize: 线程栈的大小
--分离状态属性--
int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate); 
int pthread_attr_getdetachstate(pthread_attr_t *attr, int *detachstate);
// 两个函数返回值：若成功，返回0，若出错，返回错误编号
// detachstate可以取值PTHREAD_CREATE_DETACHED 以分离状态启动线程，或PTHREAD_CREATE_JOINABLE 正常启动，应用程序可以获取线程的终止状态
// 如果对线程终止状态不感兴趣，可以用pthread_detach设置让操作系统在线程退出时收回它所占用的资源
--线程栈缓冲区大小--
int pthread_attr_setstack(pthread_attr_t *attr,void *stackaddr, size_t stacksize);
int pthread_attr_getstack(pthread_attr_t *attr,void **stackaddr, size_t *stacksize);
// 两个函数返回值：若成功，返回0，若出错，返回错误编号
// 对进程而言，虚地址空间大小固定，而这个空间被所有线程共享，如果线程数量太多，就会减小默认的线程栈大小；或者线程的函数分配了大量的自动变量或涉及很深的栈帧，则需要的栈大小可能要比默认的大
// 如果线程栈的虚地址空间用完了，可以使用malloc或mmap作为可替代的栈分配空间，并用pthread_attr_setstack改变新线程栈的栈位置
// stackaddr为线程栈的最低内存地址，但不一定是栈的开始位置，如果CPU结构是从高地址往低地址增长，stackaddr将是栈的结尾位置，而不是开始位置
--线程栈的最低地址--
int pthread_attr_setguardsize(pthread_attr_t *attr, size_t guardsize);
int pthread_attr_getguardsize(pthread_attr_t *attr, size_t *guardsize);
// 两个函数返回值：若成功，返回0，若出错，返回错误编号
// guardsize控制着线程栈末尾之后用以避免栈溢出的扩展内存大小
// guardsize设置为0，不会提供警戒缓冲区；如果修改了stackaddr，希望认为我们自己管理栈，进而使警戒缓冲区无效，相当于guardsize设置为0
--线程栈的大小--
int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize);
int pthread_attr_getstacksize(pthread_attr_t *attr, size_t *stacksize);
// 两个函数返回值：若成功，返回0，若出错，返回错误编号
// 如果希望改变默认线程栈大小，而不想自己处理线程栈的分配问题，可以使用pthread_attr_setstacksize
// 设置stacksize时，不能小于PTHREAD_STACK_MIN
// Linux/x86-32架构上，除了主线程外的所有线程，栈大小默认值为2MB
```

### 线程与信号

##### UNIX信号模型如何映射到线程中

* 信号动作、信号处置，属于进程层面
* 信号的发送既可以针对进程，也可以针对线程，满足下面三条件之一就是面向线程
  * 信号源于硬件异常如SIGBUS、SIGFPE、SIGILL、SIGSEGV
  * 当线程试图对已断开的管道操作时产生的SIGPIPE
  * 由函数pthread_kill、pthread_sigqueue所发出的信号

* 当多线程程序收到一个信号，且进程已经为信号创建了处理程序，内核会任选一条线程来处理这个信号
* 信号掩码针对每个线程而言
* 备选信号栈为每个线程特有，新建线程并不继承
* 针对为每个进程挂起的信号，以及为每条线程所挂起的信号，内核分别维护有记录，sigpending会返回整个进程和当前线程挂起信号的并集
* 如果信号处理程序中断了pthread_mutex_lock的调用，该调用总是会自动重新开始，如果中断了pthread_cond_wait，该调用要么自动重新开始（Linux如此），要么返回0，表示遭遇了假唤醒

