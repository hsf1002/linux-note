#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>


/**
 *   
 * 
 */
int
main(int argc, char *argv[])    
{
    if (argc > 2 || 0 == strcmp(argv[1], "--help"))
        fprintf(stderr, "%s filepath\n", argv[0]);
    
    if (-1 == acct(argv[1]))
        perror("acct error");

    printf("process accouting %s\n", (NULL == argv[1]) ? "disabled" : "enabled");
        
    exit(EXIT_SUCCESS);
}

