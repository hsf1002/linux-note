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
#include <pwd.h>
#include <grp.h>
#include "ugid.h"

/**
 *   
 *  获取文件stat信息
 */
int
main(int argc, char *argv[])    
{
    uid_t uid;
    gid_t gid;
    bool err;

    if (argc < 3 || 0 == strcmp(argv[1], "--help"))
        fprintf(stderr, "usage: %s owner group [file...]  owner or group can be '-' meaning leave unchanged\n", argv[0]);
    
    // 第一个参数，指定uid
    if (0 == strcmp(argv[1], "-"))
        uid = -1;
    else
        if (-1 == (uid = userid_from_name(argv[1])))
            perror("no such user");
    
    // 第一个参数，指定gid
    if (0 == strcmp(argv[2], "-"))
        gid = -1;
    else
        if (-1 == (uid = groupname_from_id(argv[2])))
            perror("no such group");

    printf("uid = %ld, gid = %ld \n", (long int)uid, (long int)gid);

    // 试图改变所有指定文件的uid和gid
    err = false;
    // 第三个参数以后，指定要修改的文件名
    for (int i=3; i<argc; ++i)
    {
        if (-1 == chown(argv[i], uid, gid))
        {
            perror("change error");
            err = true;
        }
    }

    exit(err ? EXIT_FAILURE : EXIT_SUCCESS);
}

