#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>


/**
 *   
 * 使用execlp在PATH中搜索文件

skydeiMac:27-程序的执行 sky$ which echo
/bin/echo
skydeiMac:27-程序的执行 sky$ ls -l /bin/echo
-rwxr-xr-x  1 root  wheel  18128 10 26  2017 /bin/echo
skydeiMac:27-程序的执行 sky$ echo $PATH
/Library/Frameworks/Python.framework/Versions/3.7/bin:/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin:/Users/sky/Library/Android/sdk/tools:/Users/sky/Library/Android/sdk/platform-tools://Users/sky/work/software/apache-maven-3.5.4/bin://Users/sky/software/mongodb-osx-x86_64-4.0.6/bin
skydeiMac:27-程序的执行 sky$ ./t_execlp echo
hello world, this's sky
*/
int
main(int argc, char *argv[])    
{
    char **ep;

    if (argc != 2 || 0 == strcmp(argv[1], "--help"))
        fprintf(stderr, "%s pathname\n", argv[0]);

    execlp(argv[1], argv[1], "hello world, this's sky", (char *)NULL);

    perror("execlp error");
    
    exit(EXIT_SUCCESS);
}

