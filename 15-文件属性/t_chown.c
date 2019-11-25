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

cc -g -Wall -o t_chown t_chown.c libugid.so
sudo LD_LIBRARY_PATH=. ./t_chown FENG FENG hi
uid = 1001, gid = 1002 
ll hi
-rwxrwxr-x 1 FENG FENG 20272 11月 25 10:26 hi*
sudo LD_LIBRARY_PATH=. ./t_chown hefeng hefeng hi
uid = 1000, gid = 1000 
ll hi
-rwxrwxr-x 1 hefeng hefeng 20272 11月 25 10:26 hi*

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
        if (-1 == (gid = groupid_from_name(argv[2])))
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

