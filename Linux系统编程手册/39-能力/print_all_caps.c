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

// 显示所有能力
#define PRCAP_SHOW_ALL          0x01
// 无法识别的能力
#define PRCAP_SHOW_UNRECOGNIZED 0x02

/**
 * 
 * 是否支持某项能力
 */ 
static int
cap_is_set(cap_t cap_sets, cap_value_t cap, cap_flag_t set)
{
    cap_flag_value_t value;

    if (-1 == cap_get_flag(cap_sets, cap, set, &value))
        perror("cap_get_flag error");

    return value == CAP_SET;    
}


static int
cap_is_permitted(cap_t cap_sets, cap_value_t cap)
{
    return cap_is_set(cap_sets, cap, CAP_PERMITTED);
}

static int
cap_is_effective(cap_t cap_sets, cap_value_t cap)
{
    return cap_is_set(cap_sets, cap, CAP_EFFECTIVE);
}

static int
cap_is_inheritable(cap_t cap_sets, cap_value_t cap)
{
    return cap_is_set(cap_sets, cap, CAP_INHERITABLE);
}

/**
 * 打印一行显示某项能力cap，是否在 许可、有效、可继承的能力集合cap_sets中
 * 
 */ 
static void
print_cap(cap_t cap_sets, cap_value_t cap, char *cap_str, int flag)
{
    cap_flag_value_t dummy;

    if (-1 == (cap_get_flag(cap_sets, cap, CAP_PERMITTED, &dummy)))
    {
        if ((flag & PRCAP_SHOW_ALL) || 
            cap_is_permitted(cap_sets, cap) || 
            cap_is_effective(cap_sets, cap) || 
            cap_is_inheritable(cap_sets, cap))
            printf("%-22s %s%s%s\n", cap_str, cap_is_permitted(cap_sets, cap) ? "p" : " ",
                                              cap_is_effective(cap_sets, cap) ? "e" : " ",
                                              cap_is_inheritable(cap_sets, cap) ? "i" : " ");
    }
    else
    {
        if (flag & PRCAP_SHOW_UNRECOGNIZED)
            printf("%s-22s unrecognized by libcap\n", cap_str);
    }
}

/*
    打印所有的能力，是否支持
*/
static void
print_all_caps(cap_t cap_sets, int flag)
{
    print_cap(cap_sets, CAP_AUDIT_CONTROL, "CAP_AUDIT_CONTROL", flag);
#ifdef CAP_AUDIT_READ
    print_cap(cap_sets, CAP_AUDIT_READ, "CAP_AUDIT_READ", flag);
#endif    
    print_cap(cap_sets, CAP_AUDIT_WRITE, "CAP_AUDIT_WRITE", flag);
#ifdef CAP_BLOCK_SUSPEND    
    print_cap(cap_sets, CAP_BLOCK_SUSPEND, "CAP_BLOCK_SUSPEND", flag);
#endif    
    print_cap(cap_sets, CAP_CHOWN, "CAP_CHOWN", flag);
    print_cap(cap_sets, CAP_DAC_OVERRIDE, "CAP_DAC_OVERRIDE", flag);
    print_cap(cap_sets, CAP_DAC_READ_SEARCH, "CAP_DAC_READ_SEARCH", flag);
    print_cap(cap_sets, CAP_FOWNER, "CAP_FOWNER", flag);
    print_cap(cap_sets, CAP_FSETID, "CAP_FSETID", flag);
    print_cap(cap_sets, CAP_IPC_LOCK, "CAP_IPC_LOCK", flag);
    print_cap(cap_sets, CAP_IPC_OWNER, "CAP_IPC_OWNER", flag);
    print_cap(cap_sets, CAP_KILL, "CAP_KILL", flag);
    print_cap(cap_sets, CAP_LEASE, "CAP_LEASE", flag);
    print_cap(cap_sets, CAP_LINUX_IMMUTABLE, "CAP_LINUX_IMMUTABLE", flag);
#ifdef CAP_MAC_ADMIN    
    print_cap(cap_sets, CAP_MAC_ADMIN, "CAP_MAC_ADMIN", flag);
#endif    
#ifdef CAP_MAC_OVERRIDE
    print_cap(cap_sets, CAP_MAC_OVERRIDE, "CAP_MAC_OVERRIDE", flag);
#endif    
    print_cap(cap_sets, CAP_MKNOD, "CAP_MKNOD", flag);
    print_cap(cap_sets, CAP_NET_ADMIN, "CAP_NET_ADMIN", flag);
    print_cap(cap_sets, CAP_NET_BIND_SERVICE, "CAP_NET_BIND_SERVICE", flag);
    print_cap(cap_sets, CAP_NET_BROADCAST, "CAP_NET_BROADCAST", flag);
    print_cap(cap_sets, CAP_NET_RAW, "CAP_NET_RAW", flag);
    print_cap(cap_sets, CAP_SETGID, "CAP_SETGID", flag);
    print_cap(cap_sets, CAP_SETFCAP, "CAP_SETFCAP", flag);
    print_cap(cap_sets, CAP_SETPCAP, "CAP_SETPCAP", flag);
    print_cap(cap_sets, CAP_SETUID, "CAP_SETUID", flag);
    print_cap(cap_sets, CAP_SYS_ADMIN, "CAP_SYS_ADMIN", flag);
    print_cap(cap_sets, CAP_SYS_BOOT, "CAP_SYS_BOOT", flag);
    print_cap(cap_sets, CAP_SYS_CHROOT, "CAP_SYS_CHROOT", flag);
    print_cap(cap_sets, CAP_SYS_MODULE, "CAP_SYS_MODULE", flag);
    print_cap(cap_sets, CAP_SYS_NICE, "CAP_SYS_NICE", flag);
    print_cap(cap_sets, CAP_SYS_PACCT, "CAP_SYS_PACCT", flag);
    print_cap(cap_sets, CAP_SYS_PTRACE, "CAP_SYS_PTRACE", flag);
    print_cap(cap_sets, CAP_SYS_RAWIO, "CAP_SYS_RAWIO", flag);
    print_cap(cap_sets, CAP_SYS_RESOURCE, "CAP_SYS_RESOURCE", flag);
    print_cap(cap_sets, CAP_SYS_TIME, "CAP_SYS_TIME", flag);
    print_cap(cap_sets, CAP_SYS_TTY_CONFIG, "CAP_SYS_TTY_CONFIG", flag);
#ifdef CAP_SYSLOG    
    print_cap(cap_sets, CAP_SYSLOG, "CAP_SYSLOG", flag);
#endif    
#ifdef CAP_SYSLOG
    print_cap(cap_sets, CAP_WAKE_ALARM, "CAP_WAKE_ALARM", flag);
#endif    
}

/**




 */
int main(int argc, char *argv[])
{
    cap_t cap_sets;
    char *text;

    if (argc != 2)
    {
        fprintf(stderr, "%s <textual-cap-set> \n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (NULL == (cap_sets = cap_from_text(argv[1])))
        perror("cap_from_text error");
    
    if (NULL == (text = cap_to_text(cap_sets, NULL)))
        perror("cap_to_text error");
    
    printf("cap_to_text returned %s \n", text);

    print_all_caps(cap_sets, PRCAP_SHOW_ALL);

    if (0 != cap_free(text) || 0 != cap_free(cap_sets))
        perror("cap_free error");

    exit(EXIT_SUCCESS);
}
