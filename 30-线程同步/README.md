## 第30章 线程同步

### 互斥量

线程的主要优势是通过全局变量共享信息，这种便捷性的代价是必须确保多个线程不会同时修改同一变量，互斥量本质上是一把锁，使用前必须初始化，或者用静态分配的常量PTHREAD_MUTEX_INITIALIZER，或者用pthread_mutex_init进行初始化，用pthread_mutex_destroy释放内存

```
#include <pthread.h>

int pthread_mutex_init(pthread_mutex_t * __restrict mutex,
		const pthread_mutexattr_t * __restrict attr);
int pthread_mutex_destroy(pthread_mutex_t *mutex);
// 两个函数返回值：若成功，返回0，若出错，返回错误编号
```

```
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_trylock(pthread_mutex_t *mutex);
int pthread_mutex_timedlock(pthread_mutex_t *mutex， const struct timespec *astime);
int pthread_mutex_unlock(pthread_mutex_t *mutex);
// 四个函数返回值：若成功，返回0，若出错，返回错误编号
// pthread_mutex_lock：如果互斥量当前未锁定，则锁定互斥量并立即返回，否则一直阻塞；如果被同一所有者重复加锁将导致死锁
// pthread_mutex_unlock：对未加锁的互斥量解锁或解锁由其他线程锁定的互斥量都是错误行为
// pthread_mutex_trylock：如果互斥量已经锁定，会失败并返回EBUSY错误
// pthread_mutex_timedlock：如果互斥量已经锁定，并且时间期限已满，则失败并返回ETIMEDOUT错误
```

#####类型

* PTHREAD_MUTEX_NORMAL：该类型的互斥量不具有死锁检测功能
* PTHREAD_MUTEX_RERORCHECK：该类型的互斥量都会执行错误检查
* PTHREAD_MUTEX_RECURSIVE：递归互斥量维护着一个锁计数器

##### 死锁

如果试图对一个互斥量加锁两次，其自身会陷入死锁状态；或者程序中有一个以上的互斥量，两个线程互相请求对方所拥有的资源时，也会死锁。可以通过仔细控制互斥量加锁的顺序来避免死锁，如所有线程总是在对互斥量B加锁前先锁住互斥量A，但是对互斥量的排序有时候比较困难，可以使用pthread_mutex_trylock避免死锁，如果不能获取锁，先释放已经占有的资源，做好清理工作，过一段时间再试