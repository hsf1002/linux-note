//#define _GNU_SOURCE
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
#include <sys/capability.h>
#include <sched.h>
#include <signal.h>
#include <time.h>
#include <stdarg.h>

#define err_exit(msg) do { perror(msg); exit(EXIT_FAILURE);} while (0)


/**
 * sudo apt-get install libcap-dev
 * 
cc -g -Wall -o cap_getpid cap_getpid.c -lcap

sleep 15 &
[1] 4688
./cap_getpid 4688
capabilities: =

./cap_getpid 1
capabilities: = cap_chown,cap_dac_override,cap_dac_read_search,cap_fowner,cap_fsetid,cap_kill,cap_setgid,cap_setuid,cap_setpcap,cap_linux_immutable,cap_net_bind_service,cap_net_broadcast,cap_net_admin,cap_net_raw,cap_ipc_lock,cap_ipc_owner,cap_sys_module,cap_sys_rawio,cap_sys_chroot,cap_sys_ptrace,cap_sys_pacct,cap_sys_admin,cap_sys_boot,cap_sys_nice,cap_sys_resource,cap_sys_time,cap_sys_tty_config,cap_mknod,cap_lease,cap_audit_write,cap_audit_control,cap_setfcap,cap_mac_override,cap_mac_admin,cap_syslog,cap_wake_alarm,cap_block_suspend,37+ep
 *    
 */
int
main(int argc, char *argv[])    
{
    cap_t caps;
    char *str;

    if (argc < 2)
    {
        fprintf(stderr, "usage: %s <pid>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (NULL == (caps = cap_get_pid(atoi(argv[1]))))
        err_exit("cap_get_pid");
    
    if (NULL == (str = cap_to_text(caps, NULL)))
        err_exit("cap_to_text");
    
    printf("capabilities: %s\n", str);
    
    cap_free(caps);
    cap_free(str);
    
    exit(EXIT_SUCCESS);
}
