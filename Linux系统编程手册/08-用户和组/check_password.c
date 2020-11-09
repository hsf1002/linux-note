#define _BSD_SOURCE
#define _XOPEN_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <limits.h>
#include <shadow.h>
#include <ctype.h>



/**
 *   根据shadow密码文件验证用户
 */
int
main(int argc, char *argv[])    
{
    char *username;
    char *password;
    char *encryted;
    char *p;
    struct passwd *pwd;
    struct spwd *spwd;
    bool authok;
    size_t len;
    long lnmax;

    // 确认用户名长度限制
    if (-1 == (lnmax = sysconf(_SC_LOGIN_NAME_MAX)))
        lnmax = 256;
    
    if (NULL == (username = (char *)malloc(lnmax)))
        perror("malloc error");

    printf("username: ");
    fflush(stdout);

    // 从标准输入获取用户名
    if (NULL == fgets(username, lnmax, stdin))
        exit(EXIT_FAILURE);
    
    len = strlen(username);
    if (username[len - 1] == '\n')
        username[len - 1] = '\0';

    // 根据用户名获取密码记录
    if (NULL == (pwd = getpwnam(username)))
        perror("get passwd error");
        
    // 根据用户名获取阴影记录
    spwd = getspnam(username);
    if (NULL == spwd && EACCES == errno)
        perror("no permission to read shadow file");

    // 如果阴影文件中密码记录存在，则使用阴影文件中的
    if (NULL != spwd)
        pwd->pw_passwd = spwd->sp_pwdp;

    // 根据用户输入获取输入密码
    password = getpass("password: ");
    // 将用户输入的字符串进行加密
    encryted = crypt(password, pwd->pw_passwd);
    // 对用户输入的密码进行清空
    for (p=password; *p!='\0';)
        *p++ = '\0';
    
    if (NULL == encryted)
        perror("crypt error");
    // 将用户输入的字符串加密结果与实际阴影文件的密码记录相比较
    if (!auchok = (0 == strcmp(encryted, pwd->pw_passwd)))
    {
        perror("incorrect password");
        exit(EXIT_FAILURE);
    }

    printf("successfully authenticated: UID=%ld\n", (long)pwd->pw_uid);

    /** now do authenticated work */

    exit(EXIT_SUCCESS);
}

