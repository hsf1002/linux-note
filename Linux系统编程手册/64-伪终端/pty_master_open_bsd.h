//#define _GNU_SOURCE
#define _XOPEN_SOURCE 600
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <stddef.h>
#include <signal.h>
#include <ctype.h>
#include <termios.h>
#include <errno.h>


#define PTYM_PREFIX "/dev/pty"
#define PTYS_PREFIX "/dev/tty"
#define PTY_PREFIX_LEN (sizeof(PTYM_PREFIX) + 1)
#define PTY_NAME_LEN (PTY_PREFIX_LEN + sizeof("XY"))
#define X_RANGE "pqrstuvwxyzabcde"
#define Y_RANGE "0123456789abcdef"



/**
 * 
 * 打开伪终端主设备的实现(BSD风格)
 */
int pty_master_open_bsd(char *slave_name, size_t sn_len);
