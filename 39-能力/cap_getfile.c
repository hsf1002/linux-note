//#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/times.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/capability.h>
#include <sched.h>
#include <signal.h>
#include <time.h>
#include <stdarg.h>

#define err_exit(msg) do { perror(msg); exit(EXIT_FAILURE);} while (0)


/**
 * sudo apt-get install libcap-dev
 * 
cc -g -Wall -o cap_getfile cap_getfile.c -lcap

./cap_getfile /usr/bin/sudo
No capabilities are attached to this file

*    
 */
int
main(int argc, char *argv[])    
{
    cap_t caps;
    char *str;

    if (argc < 2)
    {
        fprintf(stderr, "usage: %s <pathname>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (NULL == (caps = cap_get_file(argv[1])))
    {
        if (ENODATA == errno)
            printf("No capabilities are attached to this file\n");
        else
            err_exit("cap_get_file");
    }
    else
    {
        if (NULL == (str = cap_to_text(caps, NULL)))
            err_exit("cap_to_text");
        else
            printf("capabilities: %s\n", str);
        cap_free(str);
    }
        
    cap_free(caps);
    
    exit(EXIT_SUCCESS);
}
