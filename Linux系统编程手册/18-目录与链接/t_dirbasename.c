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
#include <libgen.h>

/**
 *   
./a.out /etc/group /usr/bin/at README.md 
/etc/group ---> /etc + group 
/usr/bin/at ---> /usr/bin + at 
README.md ---> . + README.md 
* 
 */
int
main(int argc, char *argv[])    
{
    char *t1, *t2;

    for (int i=1; i<argc; i++)
    {
        if (NULL == (t1 = strdup(argv[i])))
            perror("t1 strdup error");
        
        if (NULL == (t2 = strdup(argv[i])))
            perror("t2 strdup error");
        
        printf("%s ---> %s + %s \n", argv[i], dirname(t1), basename(t2));

        if (NULL != t1)
        {
            free(t1);
            t1 = NULL;
        }

        if (NULL != t2)
        {
            free(t2);
            t2 = NULL;
        }
    }

    exit(EXIT_SUCCESS);
}

