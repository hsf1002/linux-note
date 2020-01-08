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

