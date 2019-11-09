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

-----将time_t转换为可打印格式-----

```
#include <time.h>

char *ctime(const time_t timep);
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

