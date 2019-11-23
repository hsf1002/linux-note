### 第17章 访问控制列表

##### 概述

利用ACL（Access Control List），可以在任意数量的用户和组之中，为单个用户或组指定文件权限，装配文件系统时需要指定acl选项如mount -o acl，如果运用了ACL技术，会危及应用的移植性；一个ACL由一系列ACL记录组成，每条记录都针对单个用户或组定义了对文件的访问权限

![WechatIMG20.jpeg](https://i.loli.net/2019/11/24/e47dURYtTOjNmxz.jpg)

ACL记录由三部分构成：

* 标记类型
* 标记限定符：可选
* 权限集合

```
ACL_USER_OBJ: 文件属主的权限，只能有一条
ACL_USER：额外用户的权限，针对特定用户只能有一条
ACL_GROUP_OBJ：文件属组的权限，只能有一条
ACL_GROUP：额外组的权限，针对特定用户只能有一条
ACL_MASK：ACL_USER、ACL_GROUP_OBJ,ACL_GROUP的最大权限
ACL_OTHER：其他组的权限，只能有一条
```

最小化ACL语义上等同于传统的文件权限集合，分别是：ACL_USER_OBJ、ACL_GROUP_OBJ和ACL_OTHER，扩展ACL则是除此之外，还包括：ACL_USER、ACL_GROUP和ACL_MASK

##### ACL权限检查算法

规则顺序如下：

1. 若进程有特权，拥有所有权限
2. 若进程的有效用户ID（实际是文件系统ID）匹配文件用户ID，授予进程标记类型为ACL_USER_OBJ所指定的权限
3. 若进程的有效用户ID与某一ACL_USER的标记限定符匹配，则授予进程此记录与ACL_MASK相与&的结果
4. 若进程的组ID（有效组或任一辅助组）之一匹配于文件组（即标记类型为ACL_GROUP_OBJ），或任一ACL_GROUP的标记限定符，则进行如下检查：
   * a 若进程的组ID之一匹配于文件组，且标记类型ACL_GROUP_OBJ授予了所请求的权限，则据此判定对文件的访问权限，如果还包含了ACL_MASK，则两记录的权限相与
   * b 若进程的组ID之一匹配于ACL_GROUP的标记限定符，且该记录授予了所请求的权限，则据此判定对文件的访问权限，如果还包含了ACL_MASK，则两记录的权限相与
   * c 否则，拒绝对文件的访问
5. 否则，将以ACL_OTHER型ACE记录的权限授予进程

