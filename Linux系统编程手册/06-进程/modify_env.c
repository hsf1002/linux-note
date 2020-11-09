//#define _BSD_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


extern char **environ;

/**
 *  修改环境变量

./a.out "GREET=Hi" "HI=hi" "HELLO=hello"
GREET=Fantanstic
HI=hi
HELLO=hello 
 */
int
main(int argc, char *argv[])    
{
    // macOS未定义，清除整个环境
  //  clearenv();
    // 使用另一种方式，清除整个环境
    *environ = NULL;

    // 在命令行中逐个添加
    for (int i=1; i<argc; i++)
        if (putenv(argv[i]) != 0)
            perror("putenv error");
    // 修改其中一个
    if (setenv("GREET", "Fantanstic", 1) == -1)
        perror("setenv error");
    // 打印所有环境变量
    for (char **p=environ; *p!=NULL; p++)
        puts(*p);

    exit(EXIT_SUCCESS);
}

