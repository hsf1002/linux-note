#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>

#include <limits.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <ctype.h>
#include <stdbool.h>
#include "ugid.h"

#define MAX_LINE 1000

/**
 * 给定用户名，程序扫描所有/proc/PID/status文件以生成所有文件的列表在特定real_userid下运行的进程;对于每个进程，显示进程ID和它执行的命令
 * cc procfs_user_exe.c -o procfs_user_exe libugid.so
 
 ./procfs_user_exe hsf1002  

 ./procfs_user_exe hefeng
 2637 systemd
 2638 (sd-pam)
 2645 pulseaudio
 2647 tracker-miner-f
 2651 dbus-daemon
 2659 gnome-keyring-d
 2677 gvfsd
 2685 gvfsd-fuse
 2688 gvfs-udisks2-vo
 2702 gvfs-mtp-volume
 2706 gvfs-gphoto2-vo
 2710 gvfs-afc-volume
 2715 gvfs-goa-volume
 ..
 ..
 ..
 ..
1944656 chrome
1944680 chrome
1946483 code
1947932 chrome
1947953 chrome
1948328 chrome
1948390 cpptools-srv
1949094 procfs_user_exe


$ cat /proc/1944680/status
Name:	chrome
Umask:	0002
State:	S (sleeping)
Tgid:	1944680
Ngid:	0
Pid:	1944680
PPid:	1434473
TracerPid:	0
Uid:	1000	1000	1000	1000
Gid:	1000	1000	1000	1000
FDSize:	64
Groups:	4 24 27 30 46 120 132 133 1000 
NStgid:	1944680	10845	1
NSpid:	1944680	10845	1
NSpgid:	3009	0	0
NSsid:	3009	0	0
VmPeak:	1187402648 kB
VmSize:	1187006996 kB

..
..
..

*/
int
main(int argc, char *argv[])
{
    if (argc < 2 || strcmp(argv[1], "--help") == 0)
    {
        fprintf("%s username \n", argv[0]);
        exit(EXIT_FAILURE);        
    }

    uid_t checkedUid = userid_from_name(argv[1]);
    if (checkedUid == -1)
    {
        fprintf("Bad username: %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    DIR *dirp = opendir("/proc");
    if (dirp == NULL)
    {
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    // 扫描/proc目录
    for (;;) 
    {
        errno = 0;
        struct dirent *dp = readdir(dirp);
        if (dp == NULL) 
        {
            if (errno != 0)
            {
                perror("readdir error");
            }
            // 目录尾部
            else
                break;
        }

        // 只找目录，屏蔽已数字开头的文件
        if (dp->d_type != DT_DIR || !isdigit((unsigned char) dp->d_name[0]))
            continue;

        char path[PATH_MAX];
        snprintf(path, PATH_MAX, "/proc/%s/status", dp->d_name);

        FILE *fp = fopen(path, "r");
        // 如果进程恰好停止，可能读取失败
        if (fp == NULL)
            continue;          

        bool gotName = false;
        bool gotUid = false;
        char line[MAX_LINE], cmd[MAX_LINE];
        uid_t uid;
        while (!gotName || !gotUid) 
        {
            if (fgets(line, MAX_LINE, fp) == NULL)
                break;

            // 进程正在运行，读取名称            
            if (strncmp(line, "Name:", 5) == 0) {
                char *p;
                for (p = line + 5; *p != '\0' && isspace((unsigned char) *p); )
                    p++;
                strncpy(cmd, p, MAX_LINE - 1);
                cmd[MAX_LINE -1] = '\0';        /* Ensure null-terminated */

                gotName = true;
            }

            // 进程正在运行，读取uid，  real, effective, saved set-, and file-system user IDs
            if (strncmp(line, "Uid:", 4) == 0) {
                uid = strtol(line + 4, NULL, 10);
                gotUid = true;
            }
        }

        fclose(fp);

        // uid匹配
        if (gotName && gotUid && uid == checkedUid)
            printf("%5s %s", dp->d_name, cmd);
    }

    exit(EXIT_SUCCESS);
}