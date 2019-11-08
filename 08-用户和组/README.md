### 第8章 用户和组

##### 口令文件：/etc/passwd

用户名：加密口令：用户ID：组ID：注释字段：初始工作目录：初始shell

- 通常有一个用户名为root，用户ID为0的登录项（超级用户）
- 加密口令字段占位，包含x表示内容存储在了shadow密码文件
- 某些字段可能为空，如果加密口令字段为空，意味着该用户没有口令
- shell字段包含一个可执行程序，用作登陆shell；为空，则取系统默认，如果是/dev/null，标明是一个设备，不是可执行文件，任何人无法以该用户登陆
- 为了阻止一个特定用户登陆，除了/dev/null外，还可以将/bin/false作为登陆shell，或/bin/true
- 使用nobody用户名的目的使任何人都可以登陆，只能访问人人皆可读写的文件
- 使用finger指令支持注释字段的附加信息

##### 阴影文件：/etc/shadow

包含登录名、经过加密的密码、若干与安全性相关的字段，仅有几个用户ID为root的程序如login和passwd才可以访问

```
struct spwd 
{
    char *sp_namp; /* Login name */
    char *sp_pwdp; /* Encrypted password */
    long int sp_lstchg; /* Date of last change */
    long int sp_min; /* Minimum number of days between changes */
    long int sp_max; /* Maximum number of days between changes */
    long int sp_warn; /* Number of days to warn user to change the password */
    long int sp_inact; /* Number of days the account may be inactive */
    long int sp_expire; /* Number of days since 1970-01-01 until account expires */
    unsigned long int sp_flag; /* Reserved */
};
```

这组函数与口令文件的一组函数对应：

```
#include <shadow.h>
struct spwd *getspnam(const char *name);
struct spwd getspent(void);
// 两个函数，若成功，返回指针，若出错或到达文件尾，返回NULL

void setspent(void);
void endspent(void);
```

