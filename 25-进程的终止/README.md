### 第25章 进程的终止

##### 进程的终止：_exit和exit

```
#include <unistd.h>

void _exit(int status);
// status仅有低8位为父进程所用，0表示支持退出，非0表示异常退出，此调用总是会成功终止
```

程序一般不会直接调用 _exit，而是调用库函数exit，它在调用 _exit之前执行各种动作：

```
#include <stdlib.h>

void exit(int status);
// 执行的动作：
1. 调用退出处理程序（atexit和on_exit注册的函数）
2. 刷新stdio流缓冲区
3. 使用status提供的值执行_exit系统调用
```

return n等同于exit(n)的调用，因为main的返回值会作为exit的参数，如为显式执行return，C99标准要求，其等同于调用exit(0)

