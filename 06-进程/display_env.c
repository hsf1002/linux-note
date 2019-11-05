#include <stdio.h>
#include <stdlib.h>
#include <string.h>


extern char **environ;

/**
 *  打印参数和环境变量
 
argv[0] = ./a.out
argv[1] = hello
argv[2] = sky
TERM_PROGRAM=Apple_Terminal
TERM=xterm-256color
SHELL=/bin/bash
TMPDIR=/var/folders/3d/zndxynsd2777kl7gwlt6hnf80000gn/T/
Apple_PubSub_Socket_Render=/private/tmp/com.apple.launchd.YKCYtv51RZ/Render
TERM_PROGRAM_VERSION=404
OLDPWD=/Users/sky/work/practice/linux-note/05-深入探究文件IO
TERM_SESSION_ID=961D207E-21BD-422D-9DBD-5B457C5944E0
USER=sky
SSH_AUTH_SOCK=/private/tmp/com.apple.launchd.kOnWiaLE7S/Listeners
PATH=/Library/Frameworks/Python.framework/Versions/3.7/bin:/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin:/Users/sky/Library/Android/sdk/tools:/Users/sky/Library/Android/sdk/platform-tools://Users/sky/work/software/apache-maven-3.5.4/bin://Users/sky/software/mongodb-osx-x86_64-4.0.6/bin
PWD=/Users/sky/work/practice/linux-note/06-进程
LANG=zh_CN.UTF-8
XPC_FLAGS=0x0
XPC_SERVICE_NAME=0
HOME=/Users/sky
SHLVL=1
LOGNAME=sky
_=./a.out
 */
int
main(int argc, char *argv[])    
{
    for (int i=0; i<argc; i++)
        printf("argv[%d] = %s\n", i, argv[i]);
    
    for (char **p=environ; *p!=NULL; p++)
        puts(*p);

    exit(EXIT_SUCCESS);
}

