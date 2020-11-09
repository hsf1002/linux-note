## 第32章 线程取消

### 取消一个线程

线程可以通过pthread_cancel来请求取消同一进程中的其他线程

```
int pthread_cancel(pthread_t tid);
// 若成功，返回0，若出错，返回错误编号
```

发出请求后会立即返回，不会等待目标线程的退出

### 取消状态及类型

##### 取消状态

有两个属性没有包含在pthread_attr_t中，可取消状态和可取消类型，它们影响着线程在响应pthread_cancel调用时的行为，线程启动时默认的可取消状态是PTHREAD_CANCEL_ENABLE，设置为PTHREAD_CANCEL_DISABLE时，调用pthread_cancel并不会杀死线程，相反，取消请求对这个线程来说还处于挂起状态，当取消状态再次变成PTHREAD_CANCEL_ENABLE时，线程将在下个取消点对所有挂起的取消请求进行处理

```
int pthread_setcancelstate(int state, int * oldstate);
// 若成功，返回0，若出错，返回错误编号
#define PTHREAD_CANCEL_ENABLE        0x01  // 下个取消点生效
#define PTHREAD_CANCEL_DISABLE       0x00  // 取消推迟
```

##### 取消选项

如果取消状态为PTHREAD_CANCEL_ENABLE，则对取消请求的处理取决于取消选项：

```
int pthread_setcanceltype(int type, int *oldtype);
// 若成功，返回0，若出错，返回错误编号
#define PTHREAD_CANCEL_ASYNCHRONOUS // 可能在任何时点取消线程，场景很少
#define PTHREAD_CANCEL_DEFERED      // 取消请求保持挂起，直到到达取消点，这是新建线程的默认值
```

当某线程调用fork时，子进程会继承调用线程的取消类型及状态，当调用exec时，会将新程序主线程的取消状态和取消类型设置为：PTHREAD_CANCEL_ENABLE和PTHREAD_CANCEL_DEFERED

##### 自定义取消点

```
void pthread_testcancel(void);
```

### 取消点

若将线程的取消状态和类型分别设置为启用和延迟，当线程抵达某个取消点，取消请求才会起作用

POSIX定义了取消点和可选取消点的函数

```
// 一定会执行取消点的函数
accept	mq_timedsend	putpmsg	sigsuspend
aio_suspend	msgrcv	pwrite	sigtimedwait
clock_nanosleep	msgsnd	read	sigwait
close	msync	readv	sigwaitinfo
connect	nanosleep	recv	sleep
creat	open	recvfrom	system
fcnt12	pause	recvmsg	tcdrain
fsync	poll	select	usleep
getmsg	pread	sem_timedwait	wait
getpmsg	pthread_cond_timedwait	sem_wait	waitid
lockf	pthread_cond_wait	send	waitpid
mq_receive	pthread_join	sendmsg	write
mq_send	pthread_testcancel	sendto	writev
mq_timedreceive	putmsg	sigpause	
```

### 清理函数

线程可以设置一个或多个清理函数，当线程遭到取消时自动运行这些函数，线程终止之前可执行诸如修改全局变量、解锁互斥量等动作，与进程退出时可以用atexit函数是类似的，执行顺序与注册顺序相反

```
void pthread_cleanup_push(void (*rtn)(void *), void *arg);
void pthread_cleanup_pop(int execute);
```

当执行以下动作时，清理函数rtn由pthread_cleanup_push调度：

- 调用pthread_exit时
- 相应取消请求时
- 用非零execute参数调用pthread_cleanup_pop时
- 如果execute参数为0，则清理函数不被调用
- 如果线程是通过它的启动例程中return返回而终止，它的清理程序就不会被调用

### 异步取消

如果设置线程为可异步取消（取消类型是PTHREAD_CANCEL_ASYNCHRONOUS），可以在任意时点将其取消，而不会拖延到下一个取消点。作为一般性原则，可异步取消的线程不应该分配任何资源，也不能获取互斥量或锁，异步取消鲜有应用场景，其中之一是取消在执行计算密集型循环的线程