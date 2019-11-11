#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/utsname.h>


/**
 *   uname返回信息

node_name:   skydeiMac.local
system_name: Darwin
release:     17.7.0
version:     Darwin Kernel Version 17.7.0: Thu Jun 21 22:53:14 PDT 2018; root:xnu-4570.71.2~1/RELEASE_X86_64
machine:     x86_64

*/
int
main(int argc, char *argv[])    
{
    struct utsname uts;

    if (-1 == uname(&uts))
        perror("uname error");

    printf("node_name:   %s\n", uts.nodename);
    printf("system_name: %s\n", uts.sysname);
    printf("release:     %s\n", uts.release);
    printf("version:     %s\n", uts.version);
    printf("machine:     %s\n", uts.machine);
#if 0//def _GNU_SOURCE    
    printf("domain_name: %s\n", uts.domainname);
#endif

    exit(EXIT_SUCCESS);
}

