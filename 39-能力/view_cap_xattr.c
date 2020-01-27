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
#include <sys/types.h>
#include <sys/xattr.h>
#include <sys/capability.h>
#include <linux/capability.h>
#include <sched.h>
#include <signal.h>
#include <time.h>
#include <syslog.h>
#include <getopt.h>



/**
 * cc -g -Wall -o view_cap_xattr view_cap_xattr.c

./view_cap_xattr view_cap_xattr.c 
Untitled-1.c has no security.capability
capalibity version: 0
length of returned value = -1
    Effective bit: 0
    Permitted set: 00000000 00000000
    Inherited set: 575439a0 00400690


 */
int main(int argc, char *argv[])
{
    struct vfs_ns_cap_data cap_data;
    ssize_t value_len;

    if (argc != 2 || 0 == strcmp(argv[1], "--help"))
        fprintf(stderr, "%s <file>\n", argv[0]);
    
    if (-1 == (value_len = getxattr(argv[1], "security.capability", (char *)&cap_data, sizeof(cap_data))))
        if (errno == ENODATA)
            fprintf(stderr, "%s has no security.capability\n", argv[1]);
        else
            perror("getxattr error");
    
    // 安全能力的版本
    printf("capalibity version: %d", cap_data.magic_etc >> VFS_CAP_REVISION_SHIFT);

    // 只有VFS_CAP_REVISION_3具有rootid
    if ((cap_data.magic_etc & VFS_CAP_REVISION_MASK) == VFS_CAP_REVISION_3)
        printf("    [root ID=%u]", cap_data.rootid);
    
    printf("\n");
    printf("length of returned value = %ld\n", (long)value_len);

    printf("    Effective bit: %d\n", cap_data.magic_etc & VFS_CAP_FLAGS_EFFECTIVE);
    printf("    Permitted set: %08x %08x\n", cap_data.data[1].permitted, cap_data.data[0].permitted);
    printf("    Inherited set: %08x %08x\n", cap_data.data[1].inheritable, cap_data.data[0].inheritable);
    
    exit(EXIT_SUCCESS);
}
