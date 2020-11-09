#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

 
/**
 * 
 * 打印出目录下所有文件的绝对路径名
 */
static void
list_files(const char *dirpath)
{
    DIR *dir;
    struct dirent *dp;
    bool is_current;

    // 是否当前目录
    is_current = 0 == strcmp(dirpath, ".");

    // 打开目录流
    if (NULL == (dir = opendir(dirpath)))
        perror("opendir error");
    
    for (;;)
    {
        errno = 0;

        // 依次读取目录流中的记录
        if (NULL == (dp = readdir(dir)))
            break;
        
        // 如果是当前目录或父目录，返回
        if (0 == strcmp(dp->d_name, ".") || 0 == strcmp(dp->d_name, ".."))
            continue;

        // 如果不是当前目录，打印出目录名
        if (!is_current)
            printf("%s/", dirpath);
        // 打印出文件名
        printf("%s\n", dp->d_name);
    }

    if (0 != errno)
        perror("readdir error");
    // 关闭目录流
    if (-1 == closedir(dir))
        perror("closedir error");
}

/**
 *   
 * 
./a.out /Users/sky/work/practice/linux-note
/Users/sky/work/practice/linux-note/.DS_Store
/Users/sky/work/practice/linux-note/.git
/Users/sky/work/practice/linux-note/.gitignore
/Users/sky/work/practice/linux-note/01-历史和标准
/Users/sky/work/practice/linux-note/03-系统编程概念
/Users/sky/work/practice/linux-note/04-通用的IO模型
/Users/sky/work/practice/linux-note/05-深入探究文件IO
/Users/sky/work/practice/linux-note/06-进程
/Users/sky/work/practice/linux-note/07-内存分配
/Users/sky/work/practice/linux-note/08-用户和组
/Users/sky/work/practice/linux-note/09-进程凭证
/Users/sky/work/practice/linux-note/10-时间
/Users/sky/work/practice/linux-note/11-系统限制和选项
/Users/sky/work/practice/linux-note/12-系统和进程信息
/Users/sky/work/practice/linux-note/13-文件IO缓冲
/Users/sky/work/practice/linux-note/14-文件系统
/Users/sky/work/practice/linux-note/15-文件属性
/Users/sky/work/practice/linux-note/16-扩展属性
/Users/sky/work/practice/linux-note/17-访问控制列表
/Users/sky/work/practice/linux-note/18-目录与链接
/Users/sky/work/practice/linux-note/19-监控文件事件
/Users/sky/work/practice/linux-note/41-共享库基础
/Users/sky/work/practice/linux-note/42-共享库高级特性
/Users/sky/work/practice/linux-note/56-socket介绍
/Users/sky/work/practice/linux-note/57-socket-UNIX-Domain
/Users/sky/work/practice/linux-note/59-socket-Internet-Domain
/Users/sky/work/practice/linux-note/60-socket-服务器设计
/Users/sky/work/practice/linux-note/61-socket-高级主题
/Users/sky/work/practice/linux-note/LICENSE
/Users/sky/work/practice/linux-note/README.md

 */
int
main(int argc, char *argv[])    
{
    if (argc > 2 && 0 == strcmp(argv[1], "--help"))
        fprintf(stderr, "%s [dir...] \n", argv[0]);
    
    // 只有一个参数，列出当前目录
    if (1 == argc)
        list_files(".");
    // 列出参数所指定的目录
    else
        for (argv++; *argv; argv++)
            list_files(*argv);

    exit(EXIT_SUCCESS);
}

