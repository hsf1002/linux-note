#define _XOPEN_SOURCE 600
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
#include <dirent.h>
#include <ftw.h>

 
/**
 * 
 * 
 */
static void
usage_error(const char *prog_name, const char *msg)
{
    if (NULL != msg)
        fprintf(stderr, "%s\n", msg);
    
    fprintf(stderr, "Usage: %s [-d][-m][-p][directory-path]\n", prog_name);
    fprintf(stderr, "t-d Use FTW_DEPTH flag \n");
    fprintf(stderr, "t-m Use FTW_MOUNT flag \n");
    fprintf(stderr, "t-p Use FTW_PHYS flag \n");

    exit(EXIT_FAILURE);
}

/**
 * 
 * 
 */
static void
dir_tree(const char *pathname, const struct stat *sbuf, int type, struct FTW *ftwb)
{
    switch (sbuf->st_mode & S_IFMT)
    {
        case S_IFREG:
            printf("-");
        break;
        case S_IFDIR:
            printf("d");
        break;
        case S_IFCHR:
            printf("c");
        break;
        case S_IFBLK:
            printf("b");
        break;
        case S_IFLNK:
            printf("l");
        break;
        case S_IFIFO:
            printf("p");
        break;
        case S_IFSOCK:
            printf("s");
        break;
        
        default:
            printf("?");
        break;
    }

    printf(" %s ", (type == FTW_D) ? "D  " : (type == FTW_DNR) ? "DNR" : (type == FTW_DP) ? "DP " : \
        (type == FTW_F) ? "F  " : (type == FTW_SL) ? "SL " : (type == FTW_SLN) ? "SLN" : (type == FTW_NS) ? "NS " : "   ");
    
    // 调用stat成功，打印出i-node编号
    if (type != FTW_NS)
        printf("%7ld ", (long)sbuf->st_ino);
    // 调用stat失败
    else
        printf("       ");
    // 打印出层级
    printf(" %*s", 4 * ftwb->level, "");
    // 打印出文件名
    printf("%s\n", &pathname[ftwb->base]);

    return 0;
}

/**
 *   
 * 使用nftw遍历文件树

skydeiMac:18-目录与链接 sky$ ./a.out .
d D   7284699  .
- F   7291791      a.out
- F   7288106      get_num.h
- F   7288113      libgetnum.so
- F   7290047      list_files.c
- F   7291788      nftw_dir_tree.c
- F   7291059      README.md
- F   7288795      t_unlink
- F   7290069      t_unlink.c
d D   7288796      t_unlink.dSYM
d D   7288797          Contents
- F   7288800              Info.plist
d D   7288798              Resources
d D   7288799                  DWARF
- F   7288801                      t_unlink
skydeiMac:18-目录与链接 sky$ ./a.out -d .
- F   7291791      a.out
- F   7288106      get_num.h
- F   7288113      libgetnum.so
- F   7290047      list_files.c
- F   7291788      nftw_dir_tree.c
- F   7291059      README.md
- F   7288795      t_unlink
- F   7290069      t_unlink.c
- F   7288800              Info.plist
- F   7288801                      t_unlink
d DP  7288799                  DWARF
d DP  7288798              Resources
d DP  7288797          Contents
d DP  7288796      t_unlink.dSYM
d DP  7284699  .
 */
int
main(int argc, char *argv[])    
{
    int flags;
    int opt;

    flags = 0;

    while ((opt = getopt(argc, argv, "dmp")) != -1)
    {
        switch (opt)
        {
            // 后序遍历，默认是前序遍历
            case 'd':
                flags |= FTW_DEPTH;
            break;
            // 表示不会越界进入另一系统
            case 'm':
                flags |= FTW_MOUNT;
            break;
            // 表示不对符号链接进行解引用
            case 'p':
                flags |= FTW_PHYS;
            break;
            default:
            break;
        }
    }

    if (argc > optind + 1)
        usage_error(argv[0], NULL);
    
    if (nftw((argc > optind) ? argv[optind] : ".", dir_tree, 10, flags) == -1)
        perror("nftw error");

    exit(EXIT_SUCCESS);
}

