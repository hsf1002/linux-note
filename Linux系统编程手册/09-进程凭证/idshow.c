#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <limits.h>
#include <sys/fsuid.h>
#include <ctype.h>
#include "ugid.h"

#define SG_SIZE (NGROUPS_MAX + 1)

/**
 *   显示进程的所有用户ID和组ID

UID: real id = hefeng (1000); effe id = hefeng (1000); save id = hefeng (1000); file id = hefeng (1000) 
GID: real id = hefeng (1000); effe id = hefeng (1000); save id = hefeng (1000); file id = hefeng (1000) 
supplementary groups 10 
adm (4)cdrom (24)sudo (27)dip (30)plugdev (46)lpadmin (113)sambashare (128)vboxusers (129)hefeng (1000)usbfs (1001)

*/
int
main(int argc, char *argv[])    
{
    uid_t ruid;
    uid_t euid;
    uid_t suid;
    uid_t fsuid;
    gid_t rgid;
    gid_t egid;
    gid_t sgid;
    gid_t fsgid;
    gid_t support_groups[SG_SIZE];
    int num_groups;
    int i;
    char *p;

    if (-1 == getresuid(&ruid, &euid, &suid))
        perror("getresuid error");
    if (-1 == getresgid(&rgid, &egid, &sgid))
        perror("getresgid error");

    // 非特权进程试图修改文件系统ID总被忽略，即便如此，如下调用依然返回当前文件系统ID
    fsuid = setfsuid(0);
    fsgid = setfsgid(0);

    printf("UID: ");

    p = username_from_id(ruid);
    printf("real id = %s (%ld); ", (p == NULL) ? "???": p, (long)ruid);
    p = username_from_id(euid);
    printf("effe id = %s (%ld); ", (p == NULL) ? "???": p, (long)euid);
    p = username_from_id(suid);
    printf("save id = %s (%ld); ", (p == NULL) ? "???": p, (long)suid);
    p = username_from_id(fsuid);
    printf("file id = %s (%ld) \n", (p == NULL) ? "???": p, (long)fsuid);

    printf("GID: ");
    p = groupname_from_id(rgid);
    printf("real id = %s (%ld); ", (p == NULL) ? "???": p, (long)rgid);
    p = groupname_from_id(egid);
    printf("effe id = %s (%ld); ", (p == NULL) ? "???": p, (long)egid);
    p = groupname_from_id(sgid);
    printf("save id = %s (%ld); ", (p == NULL) ? "???": p, (long)sgid);
    p = groupname_from_id(fsgid);
    printf("file id = %s (%ld) \n", (p == NULL) ? "???": p, (long)fsgid);

    if (-1 == (num_groups = getgroups(SG_SIZE, support_groups)))
        perror("getgroups error");

    printf("supplementary groups %d \n", num_groups);

    for(int i=0; i<num_groups; ++i)
    {
        p = groupname_from_id(support_groups[i]);
        printf("%s (%ld)", (p == NULL) ? "???" : p, (long)support_groups[i]);
    }

    printf("\n");

    exit(EXIT_SUCCESS);
}

