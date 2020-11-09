#define _GNU_SOURCE
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
#include <signal.h>


/**
 * 统计信号集中信号的个数
 */
void
print_sigset(FILE *of, const char *prefix, const sigset_t *sigset);

/**
 * 
 * 统计处于阻塞状态的信号个数
 */
int
print_sigmask(FILE *of, const char *msg);

/**
 * 
 * 统计处于pending状态的信号个数
 */
int 
print_sigpending(FILE *of, const char *msg);
