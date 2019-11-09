### 第9章 进程凭证

##### 概述

每个进程都有一套用数字表示的用户ID，组ID，有时候将这些ID称之为进程凭证：

* 实际用户ID（real user ID）和实际组ID（real group ID）
* 有效用户ID（effective user ID）和有效组ID（effective group ID）
* 保存的set-user-id（saved set-user-ID）和保存的set-group-id（saved set-group-ID）
* 文件系统用户ID（file-system user ID）和文件系统组ID（file-system group ID）
* 辅助组ID

##### 实际用户ID和实际组ID

确定了进程所属的用户和组

##### 有效用户ID和有效组ID

有效用户ID和组ID和相对于的实际ID相等，但有两种方法使其不同：系统调用或执行set-user-ID和set-group-ID程序

##### set-user-ID和set-group-ID程序

set-user-ID程序会将进程的有效用户ID设置为可执行文件的用户ID，从而获得常规情况下并不具备的权限，set-group-ID程序对进程有效组ID实现类似任务

```
ls -l prog
-rwxr-xr-x

chmod u+s prog	// 打开set-user-ID权限位
chmod g+s prog	// 打开set-grop-ID权限位
```

打开权限位之后，可执行权限x标识会被s标识替换

```
ls -l prog
-rwsr-sr-s
```

运行set-user-ID的程序时，内核会将有效用户ID设置为可执行文件的用户ID，set-group-ID程序对进程有效组ID的操作类似，通过这种方式修改进程的有效用户ID或组ID，能够使进程获取常规情况下不具备的权限；如果一个可执行文件的属主是root，为此程序设置了set-user-ID权限位，运行程序时，进程会取得超级用户权限；也可利用此机制，将进程的有效ID修改为root之外的其他用户，如要访问一个受保护文件，可以创建一个具有对该文件访问权限的专有用户（组）ID，再创建一个set-user-ID（set-group-ID）程序，将进程有效用户（组）ID变更为这个专用ID，这样无需拥有超级用户的所有权限，程序就能访问该文件

Linux系统中常用的set-user-ID程序包括passwd：修改用户密码，mount和unmount：加载卸载文件系统，su：允许用户以另一个身份运行shell

```
su
password:
chown root check_password // 修改程序所属为root
chmod u+s check_password  // 设置set-user-ID权限位有效
ls -al check_password
-rwsr-xr-x
exit
whoami			// 并非root用户
mtk
./check_password	// 却可以访问shadown文件
username:avr
password:
successfully authenticated: UID=1001
```

set-user-ID和set-group-ID技术集实用性和强大功能于一身，但一旦涉及欠佳可能造成安全隐患

##### 保存set-user-ID和保存set-group-ID

当执行程序时，依次发生如下事件：

1. 若可执行文件的set-user-ID权限位开启，则将进程的有效用户（组）ID设置为可执行文件的属主，若未开启此权限位，则进程的有效用户（组）ID保持不变
2. 保持set-user-ID和保存set-group-ID的值由对应的有效ID复制而来，无论是否开启了set-user-ID和set-group-ID权限位

假如某程序的实际用户ID、有效用户ID、保存set-user-ID都是1000，当其执行了root用户拥有的set-user-ID程序后，进程的用户ID将发生变化：

```
real=1000 effective=0 saved=0
```

