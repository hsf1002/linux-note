#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mount.h>

/**
 * 
 */
static void
usage_error(const char *prog_name, const char *msg)
{
    if (NULL != msg)
    {
        fprintf(stderr, "%s", msg);
    }

    fprintf(stderr, "usage: %s [options] source target\n\n", prog_name);
    fprintf(stderr, "available options:\n");
#define fpe(str) fprintf(stderr, "    " str)
    fpe("-t fstype    [e.g: 'ext2' or 'reiserfs']\n");
    fpe("-o data      [file system-depentdent options, \n");
    fpe("             [e.g: 'bsdgroups' for ext2]\n");
    fpe("-f mountflags can include any of: \n");
#define fpe2(str) fprintf(stderr, "        " str)
    fpe2("b - MS_BIND    create a bind mount\n");
    fpe2("d - MS_DIRSYNC synchronous directory update\n");
    fpe2("l - MS_MANDLOCK permit mandatory locking\n");
    fpe2("m - MS_MOVE    atomically move subtree\n");
    fpe2("A - MS_NOATIME donot update atime\n");
    fpe2("V - MS_NODEV   donot permit device access\n");
    fpe2("D - MS_NODIRATIME donot update atime on directories\n");
    fpe2("E - MS_NOEXEC  donot allow executable\n");
    fpe2("S - MS_NOSUID  disable set-user/group ID programs\n");
    fpe2("r - MS_RDONLY  read only mount\n");
    fpe2("c - MS_REC     recursive mount\n");
    fpe2("R - MS_REMOUNT remount\n");
    fpe2("s - MS_SYNCHRONOUS make writes synchronous\n");

    exit(EXIT_SUCCESS);
}

/**
 *   
 *  使用mount
 
mkdir /testfs
./t_mount -t ext2 -o bsdgroups /dev/sda12 /testfs
cat /proc/mounts | grep sda12
grep sda12 /etc/mtab   // 未产生任何输出，程序不会更新该文件

./t_mount -f Rr /dev/sda12 /testfs // 以只读方式挂载
cat /proc/mounts | grep sda12       // 查看改变

mkdir /demo
./t_mount -f m /testfs /demo       // 将挂载点移动到新位置
cat /proc/mounts | grep sda12       // 查看改变
 */
int
main(int argc, char *argv[])    
{
    unsigned long flags;
    char *data;
    char *fstype;
    int opt;

    flags = 0;
    data = NULL;
    fstype = NULL;
    // "a:b:cd::e"，这就是一个选项字符串。对应到命令行就是-a ,-b ,-c ,-d, -e 。冒号表示参数，一个冒号就表示这个选项后面必须带有参数（没有带参数会报错哦），但是这个参数可以和选项连在一起写，也可以用空格隔开，比如-a123 和-a   123（中间有空格） 都表示123是-a的参数；两个冒号的就表示这个选项的参数是可选的，即可以有参数，也可以没有参数，但要注意有参数时，参数与选项之间不能有空格（有空格会报错的哦）
    while ((opt = getopt(argc, argv, "o:t:f:")) != -1)
    {
        switch (opt)
        {
            // optarg表示选项参数
            case 'o':
            {
                data = optarg;
            }
            break;
            case 't':
            {
                fstype = optarg;
            }
            break;
            case 'f':
            {
                for (int i=0; i<strlen(optarg); ++i)
                {
                    switch (optarg[i])
                    {
                        case 'b':
                        {
                            flags |= MS_BIND;
                        }
                        break;
                        case 'd':
                        {
                            flags |= MS_DIRSYNC;
                        }
                        break;
                        case 'l':
                        {
                            flags |= MS_MANDLOCK;
                        }
                        break;
                        case 'm':
                        {
                            flags |= MS_MOVE;
                        }
                        break;
                        case 'A':
                        {
                            flags |= MS_NOATIME;
                        }
                        break;
                        case 'V':
                        {
                            flags |= MS_NODEV;
                        }
                        break;
                        case 'D':
                        {
                            flags |= MS_NODIRATIME;
                        }
                        break;
                        case 'E':
                        {
                            flags |= MS_NOEXEC;
                        }
                        break;
                        case 'S':
                        {
                            flags |= MS_NOSUID;
                        }
                        break;
                        case 'r':
                        {
                            flags |= MS_RDONLY;
                        }
                        break;
                        case 'c':
                        {
                            flags |= MS_REC;
                        }
                        break;
                        case 'R':
                        {
                            flags |= MS_REMOUNT;
                        }
                        break;
                        case 's':
                        {
                            flags |= MS_SYNCHRONOUS;
                        }
                        break;
                        
                        default:
                        {
                            usage_error(argv[0], NULL);
                        }
                        break;
                    }
                }
            }
            break;
            default:
            {
                usage_error(argv[0], NULL);
            }
            break;
        }
    }

    // optind是下一个检索位置
    if (argc != optind + 2)
    {
        usage_error(argv[0], "Wrong number of arguments\n");
    }

    if (-1 == (mount(argv[optind], argv[optind + 1], fstype, flags, data)))
    {
        perror("mount error");
    }

    exit(EXIT_SUCCESS);
}

