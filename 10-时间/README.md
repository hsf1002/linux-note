### 第10章 时间

##### 日历时间

通用协调时间UTC，即格林尼治时间GMT，以1970年1月1日开始到当前时间经过的秒数，用time_t表示：

```
#include<time.h>    
int gettimeofday(struct timeval *restrict tp, void *restrict tzp);
// 总是返回0，tzp的唯一合法值是NULL
// 提供了比time更高的精度（微秒）

struct timeval {
    long tv_sec;        /* seconds */
    long tv_usec;       /* microseconds */
};
```

```
include<time.h>       
time_t time(time_t *calptr);
// 若成功，返回时间值，若出错，返回-1
// 如果参数为非空，时间值也存放在calptr中

typedef long     time_t;    /* 时间值time_t 为长整型的别名*/
```

