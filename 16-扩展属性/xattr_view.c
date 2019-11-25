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
#include <sys/xattr.h>

#define XATTR_SIZE 10000


static void
usage_error(const char *name)
{
    fprintf(stderr, "usage: %s [-x] file... \n", name);
    exit(EXIT_FAILURE);
}

/**
 *   
 *  获取文件的扩展属性EA
 * 
setfattr -n user.x -v 'the past is past' hi
setfattr -n user.y -v 'the past is not the past' hi
./a.out hi
hi: 
    name = user.x; value = the past is past
    name = user.y; value = the past is not the past

 */
int
main(int argc, char *argv[])    
{
    char list[XATTR_SIZE];
    char value[XATTR_SIZE];
    ssize_t list_len;
    ssize_t value_len;
    int ns;
    int opt;
    bool hex;

    // 是否以16进制显示
    hex = false;
    // 判断命令行参数是否有x，用来以16进制显示
    while ((opt = getopt(argc, argv, "x")) != -1)
    {
        switch (opt)
        {
            case 'x':
                hex = true;
            break;
            case '?':
                usage_error(argv[0]);
            break;
        }
    }

    if (optind >= argc)
        usage_error(argv[0]);
    
    // 对所有文件进行获取EA的操作
    for (int i=optind; i<argc; ++i)
    {
        if (-1 == (list_len = listxattr(argv[i], list, XATTR_SIZE)))
            perror("listxattr error");
        
        printf("%s: \n", argv[i]);

        // 获取每个文件所有的EA
        for (ns=0; ns<list_len; ns+=strlen(&list[ns]) + 1)
        {
            printf("    name = %s; ", &list[ns]);

            if (-1 == (value_len = getxattr(argv[i], &list[ns], value, XATTR_SIZE)))
                perror("couldnot get value");
            else if (!hex)
                printf("value = %.*s", (int)value_len, value);
            else
            {
                printf("value = ");

                for (int k=0; k<value_len; k++)
                    printf("%02x ", (unsigned int)value[k]);
            }
            printf("\n");
        }

        printf("\n");
    }
    
    exit(EXIT_SUCCESS);
}

