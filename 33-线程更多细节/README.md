## 线程：更多细节

### 线程限制

可以通过sysconf函数查看线程相关限制

```
PTHREAD_DESTRUCTOR_ITERATIONS: 销毁一个线程数据最大的尝试次数，通过_SC_THREAD_DESTRUCTOR_ITERATIONS查询
PTHREAD_KEYS_MAX: 一个进程可以创建的最大key的数量,通过_SC_THREAD_KEYS_MAX查询 
PTHREAD_STACK_MIN: 线程可以使用的最小的栈空间大小, 通过_SC_THREAD_STACK_MIN查询
PTHREAD_THREADS_MAX:一个进程可以创建的最大的线程数, 通过_SC_THREAD_THREADS_MAX查询
```

Linux/x86-32架构上，除了主线程外的所有线程，栈大小默认值为2MB

