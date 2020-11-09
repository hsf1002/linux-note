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

```
struct passwd 
{ 
    char * pw_name; /* Username */ 
    char * pw_passwd; /* Password */ 
    __uid_t pw_uid; /* User ID */ 
    __gid_t pw_gid; /* Group ID */ 
    char * pw_gecos; /* Real Name or Comment field */ 
    char * pw_dir; /* Home directory */ 
    char * pw_shell; /* Shell Program */ 
}; 
```

从口令文件中获取记录：

```
#include <pwd.h>

struct passwd *getpwnam(const char *name);
struct passwd *getpwuid(uid_t uid);
// 返回值：若成功，返回一个指向一条记录的指针，若出错，返回NULL
// 当未启用shadow密码的情况下，pw_passwd才会包含有效信息
```

对于出错和“未发现匹配记录”需要加以区分：

```
struct passwd *pwd;

errno = 0;
pwd = getpwnam(name);

if (NULL == pwd)
{
	if (0 == errno)
		// not found;
	else
		// error
}
```

按顺序扫描口令文件中的记录：

```
#include <pwd.h>
struct passwd getpwdent(void);
// 两个函数，若成功，返回指针，若出错或到达文件尾，返回NULL

void setpwent(void);	// 重返文件的起始处
void endpwent(void);
```

如下代码遍历整个密码文件，打印出登录名和用户ID：

```
struct passwd *pwd;

while ((pwd = getpwent()) != NULL)
	printf(""%-8s %5ld\n", pwd->pw_name, (long)pwd->pw_uid);
endpwent();
```

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

##### 组文件：/etc/group

系统中每个组在组文件中对应一条记录，包含四个字段：

* 组名
* 经过加密的密码，现代UNIX系统很少使用，存放于/etc/gshadow中，仅供由特权的用户和程序访问
* 组ID
* 用户列表

为了证明用户avr是users、staff、teach三个组的成员，首先在口令文件中有记录：

```
avr:x:1001:100:Anthony Robins:/home/avr:/bin/bash
```

其中组ID是100，用户名是avr，则在组文件中应有如下记录：

```
users:x:100:
staff:x:101:mtk,avr,martinl
teach:x:104:avr,alc
```

```
struct group
{
		char *gr_name;		/* group name */
		char *gr_passwd;  /* encrypted password if not password shadowing */
		gid_t gr_gid;     /* group id */
		char ** gr_mem;   /* NULL-terminated array of pointers to names of members listed in /etc/group */
}
```

查看组名或组ID：

```
#include<grp.h>
struct group *getgrpid(gid_t gid);
struct group *getgrnam(const char *name);
// 两个函数，若成功，返回指针，若出错，返回NULL
```

如果要搜索整个组文件：

```
#include<grp.h>
struct group *getgrent();
// 若成功，返回指针，若出错或到达文件尾，返回NULL
void setgrent();
void endgrent();
```

##### 密码加密和用户认证

UNIX采用单向加密对密码进行加密，所以由密码的加密形式无法还原出原始密码，验证候选密码的唯一方法是使用同一算法对其加密，并将其与/etc/shadow的密码进行匹配，加密算法封装于crypt函数

```
#define _XOPEN_SOURCE
#include <unistd.h>

char *crypt(const char *key, const char *salt);
// 返回值：若成功，返回长度为13个字符的静态字符串（头两个字符是对salt的拷贝），若出错，返回NULL
// key最长为8字符，salt是2个字符，用来扰动（改变）DES算法
// 要使用需在编译时开启-lcrypt选项
// MD5是一种复杂的哈希函数，返回34个字符，两种方法殊途同归，对输入的密码既不可逆又难以破解
// 解密后的明文在使用后应立即从内存删除，防止恶意之徒读取内核转储文件以获取密码
```

getpass首先屏蔽回显功能，并停止对终端特殊字符的处理（一般是CTRL+C），然后打印出prompt所指向的字符串，读取一行输入，返回以NULL结尾的输入字符串（剥离尾部的换行符）：

```
#define _BSD_SOURCE
#include <unistd.h>

char *getpass(const char *prompt);
// 返回值：若成功，返回输入密码的静态字符串，若出错，返回NULL
```

