### 第59章 SOCKET: Internet Domain

##### Internet Domain socket

Internet domain流socket基于TCP，提供可靠的双向的字节流通信信道；Internet domain数据报socket基于UDP，它与UNIX domain的数据报有差异：

* UNIX domain数据报socket是可靠的，但UDP则不可靠-数据报可能丢失、重复、到达顺序与发送顺序不一致
* UNIX domain数据报socket发送数据，在接收数据队列为满时阻塞，使用UDP接收队列满，数据报会被默默丢弃

##### 网络字节序

```
#include <arpa/inet.h>

uint32_t htonl(uint32_t hostint32); // 返回网络字节序表示的32位整数
uint16_t htons(uint16_t hostint16); // 返回网络字节序表示的16位整数
uint32_t ntohl(uint32_t netint32;   // 返回主机字节序表示的32位整数
uint16_t ntohs(uint16_t netint16);  // 返回主机字节序表示的16位整数

h: 主机host
n: 网络network
l: 长整型，4字节
s: 短整型，2字节
```

