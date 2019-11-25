#include <stdio.h>
#include <stdlib.h>
#include <sys/xattr.h>

#define MAX_NAME  256


/**
 * 
 * xattr实例
*/
int
main(int argc, char *argv[])
{
    char *value;
    char v[MAX_NAME];

    if (argc < 2 || strcmp(argv[1], "--help") == 0)
        fprintf(stderr, "%s file\n", argv[0]);

    value = "The past is not dead.";
    if (setxattr(argv[1], "user.x", value, strlen(value), 0) == -1)
        perror("setxattr x error");

    value = "In fact, it's not even past.";
    if (setxattr(argv[1], "user.y", value, strlen(value), 0) == -1)
        perror("setxattr y error");

    memset(v, 0x00, MAX_NAME);
    if (-1 == getxattr(argv[1], "user.x", v, MAX_NAME))
        perror("getxattr x error");
    else
        printf("user.x = %s\n", v);

    memset(v, 0x00, MAX_NAME);
     if (-1 == getxattr(argv[1], "user.y", v, MAX_NAME))
        perror("getxattr y error");
    else
        printf("user.y = %s\n", v);
    
    if (-1 == removexattr(argv[1], "user.x"))
        perror("removexattr x error");
    if (-1 == removexattr(argv[1], "user.y"))
        perror("removexattr y error");    

    exit(EXIT_SUCCESS);
}
