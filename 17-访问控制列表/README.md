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