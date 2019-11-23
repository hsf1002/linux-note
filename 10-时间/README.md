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

##### 时间转换函数

![WechatIMG11.jpeg](https://i.loli.net/2019/11/23/CbN6gFySUkQ85dx.jpg)

-----将time_t转换为可打印格式-----

```
#include <time.h>

char *ctime(const time_t *timep);
// 返回一个长达26个字符的字符串，内含标准格式的日期和时间，如 Wed Jun 8 14:22:34 2011
// 会自动考虑本地时区和DST设置
```

-----time_t和分解时间之间的转换-----

time_t->分解时间：

```
#include<time.h>
struct tm *gmtime(const time_t *calptr);  		
// 转化为国际标准时间的年、月、日、时、分、秒等，若出错，返回NULL
struct tm *localtime(const time_t *calptr);     
// 转化为本地时间(考虑到本地时区和夏令时标志)，若出错，返回NULL

struct tm {
   int tm_sec;    /* Seconds (0-60) */
   int tm_min;    /* Minutes (0-59) */
   int tm_hour;   /* Hours (0-23) */
   int tm_mday;   /* Day of the month (1-31) */
   int tm_mon;    /* Month (0-11) */
   int tm_year;   /* Year since 1900 */
   int tm_wday;   /* Day of the week (0-6, Sunday = 0) */
   int tm_yday;   /* Day in the year (0-365, 1 Jan = 0) */
   int tm_isdst;  /* Daylight saving time */
};
```

分解时间->time_t：

```
#include<time.h>  
time_t mktime(struct tm *tmptr);
// 若成功，返回日历时间，若出错，返回-1
```

-----分解时间和可打印格式之间的转换-----

分解时间->可打印格式：

```
char *asctime(const struct tm *timeptr);
// 返回值：若成功，返回一个静态字符串，若出错，返回NULL
// 同ctime一样，也无法控制生成字符串的格式
```

```
size_t strftime(char *restrict buf, size_t maxsize, const char *restrict format, const struct tm *restrict tmptr);
size_t strftime(char *restrict buf, size_t maxsize, const char *restrict format, const struct tm *restrict tmptr, locale_t locale);
// 两个函数，若有空间，返回存入数组的字符数，否则，返回0
// 提供比asctime更精确的控制

%a	Abbreviated weekday name *	Thu
%A	Full weekday name *	Thursday
%b	Abbreviated month name *	Aug
%B	Full month name *	August
%c	Date and time representation *	Thu Aug 23 14:55:02 2001
%C	Year divided by 100 and truncated to integer (00-99)	20
%d	Day of the month, zero-padded (01-31)	23
%D	Short MM/DD/YY date, equivalent to %m/%d/%y	08/23/01
%e	Day of the month, space-padded ( 1-31)	23
%F	Short YYYY-MM-DD date, equivalent to %Y-%m-%d	2001-08-23
%g	Week-based year, last two digits (00-99)	01
%G	Week-based year	2001
%h	Abbreviated month name * (same as %b)	Aug
%H	Hour in 24h format (00-23)	14
%I	Hour in 12h format (01-12)	02
%j	Day of the year (001-366)	235
%m	Month as a decimal number (01-12)	08
%M	Minute (00-59)	55
%n	New-line character ('\n')	
%p	AM or PM designation	PM
%r	12-hour clock time *	02:55:02 pm
%R	24-hour HH:MM time, equivalent to %H:%M	14:55
%S	Second (00-61)	02
%t	Horizontal-tab character ('\t')	
%T	ISO 8601 time format (HH:MM:SS), equivalent to %H:%M:%S	14:55:02
%u	ISO 8601 weekday as number with Monday as 1 (1-7)	4
%U	Week number with the first Sunday as the first day of week one (00-53)	33
%V	ISO 8601 week number (00-53)	34
%w	Weekday as a decimal number with Sunday as 0 (0-6)	4
%W	Week number with the first Monday as the first day of week one (00-53)	34
%x	Date representation *	08/23/01
%X	Time representation *	14:55:02
%y	Year, last two digits (00-99)	01
%Y	Year	2001
%z	ISO 8601 offset from UTC in timezone (1 minute=1, 1 hour=100)
If timezone cannot be termined, no characters	+100
%Z	Timezone name or abbreviation *
If timezone cannot be termined, no characters	CDT
%%	A % sign	%
```

可打印格式->分解时间：

```
char *strptime(const char *str, const char *format, struct tm *timeptr);
// 返回值：若成功，返回str的下一个未处理字符的位置，若出错（如无法匹配格式字符串），返回NULL
// 是strftime的逆函数
```

##### 时区

位于/usr/share/zoneinfo中，系统的本地时间由时区文件/etc/localtime定义，通常链接到/usr/share/zoneinfo下的一个文件，为运行中的程序指定时区，需要将TZ环境变量设置为由一冒号：和时区名称组成的字符串，另一种方法是将一个格式字符串赋值给TZ，如：

```
TZ="CET-1:00:00CEST-2:00:00,M3.5.0,M10.5.0"
但其缺乏可读性，在Linux下可以如下设置：
TZ=":Europe/Berlin"
```

##### 地区

locale包括语言、数字、货币、日期和时间之类的信息，维护在文件/usr/share/local，其下每个子目录都包含一特定地区的信息：

```
language[_territory[.condeset]][@modifier]
```

language是双字母的ISO语言代码，territory是双字母的ISO国家代码，codeset表示字符编码集，modifier提供了一种方法，用以区分多个目录下的language、territory、codeset相同的情况

```
de_DE.uft-8@euro表示德语、德国、UTF-8编码，欧元为货币
```

locale -a列出系统上定义的整套地区，函数setlocale既可以查询又可以设置当前地区：

```
#include <locale.h>

char *setlocale(int category, const char *locale);
// 返回值：若成功，返回一个指针指向标识这个地区设置的字符串，如果只是查而不该，则把locale设置为NULL，若出错，返回NULL
category的取值（可以使用LC_ALL设置所有的值）：
LC_CTYPE: 该文件包含字符分类
LC_COLLATE: 该文件包含字符集的排序规则
LC_MONETARY: 该文件包含币值的格式化规则
LC_NUMERIC: 该文件包含对币值以外数字的格式化规则
LC_TIME: 该文件包含对日期和时间的格式化规则
LC_MESSAGES: 该文件包含针对肯定和否定响应，就格式及数值做了规定
// locale若是字符串，可能是en_US或de_DE，若是空字符串，则从环境变量中取得地区的设置
// LANG的优先级最低，通常使用LANG为所有内容设置默认值，再用单独的LC*变量设置
```

##### 更新系统时钟

settimeofday和adjtime这两个接口通常由工具软件维护，应用程序很少用到，其中settimeofday是gettimeofday的逆函数：

```
#define _BSD_SOURCE
#include <sys/time.h>

int settimeofday(const struct timeval *tv, const struct timezone *tz);
// 返回值：若成功，返回0，若出错，返回-1
// tz已被废弃，应该始终置为NULL
// Linux还提供了stime设置系统时钟，它允许使用秒的精度表示新的日志时间
```

settimeofday可能造成系统时间的突然变化，可能对依赖系统时钟单调递增的应用造成有害影响，通常推荐使用库函数adjtime，它将系统时钟逐步调整到正确的时间：

```
#define _BSD_SOURCE
#include <sys/time.h>

int adjtime(struct timeval *delta, struct timeval *olddelta);
// 返回值：若成功，返回0，若出错，返回-1
// delta指定需要改变时间的秒和微秒，为正数表示系统时间会额外快一点点，直到增加完所需的时间，为负数则减慢
// 在adjtime执行的时间里，可能无法完成时钟调整，剩余未经调整的时间会保存在olddelta，如不关心，指定为NULL
```

##### 软件时钟

时间相关的各种系统调用的精度是受限于系统软件时钟的分辨率，它的度量单位是jiffies，内核源码中是常量HZ，这是内核按照round-robin的分时调度算法分配CPU进程的单位，如软件时钟速度是100赫兹，一个jiffies是10毫秒，Linux x86-32 2.6.0内核的软件时钟速度已经提高到了1000赫兹

##### 进程时间

可分为两部分：

* 用户CPU时间，也称虚拟时间
* 系统CPU时间

系统调用times，检索进程时间信息：

```
#include <sys/time.h>
clock_t times(struct tms *buf);
// 返回值，若成功，返回自过去的任意点流逝的以时钟计时单元为单位的时间，若出错，返回-1
// Linux上，buf可以为NULL，但是返回值将没有意义
// 数据类型clock_t是用时钟计时单元clock tick为单位度量时间的整数值，可以使用sysconf(_SC_CLK_TCK)来获取每秒包含的时钟计时单元数，再除以clock_t转换为秒

struct tms
{
	clock_t tms_utime;	// user CPU time by the caller
	clock_t tms_stime;  // system CPU time by the caller
	clock_t tms_cutime; // user CPU time of all (waited for) children
	clock_t tms_cstime; // system CPU time of all (waited for) children
}
```

函数clock提供了一个简单的接口用于获取进程时间：

```
include <time.h>

clock_t clock(void);
// 返回值：若成功，返回进程总CPU时间，若出错，返回-1
```

