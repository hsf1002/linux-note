#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>


#ifdef __GNUC__
__attribute__((__noreturn__))
#endif

/* ename.c.inc

   Built on GNU/Linux x86_64 with glibc 2.28
*/
static char *ename[] = {
    /*   0 */ "", 
    /*   1 */ "EPERM", "ENOENT", "ESRCH", "EINTR", "EIO", "ENXIO", 
    /*   7 */ "E2BIG", "ENOEXEC", "EBADF", "ECHILD", 
    /*  11 */ "EAGAIN/EWOULDBLOCK", "ENOMEM", "EACCES", "EFAULT", 
    /*  15 */ "ENOTBLK", "EBUSY", "EEXIST", "EXDEV", "ENODEV", 
    /*  20 */ "ENOTDIR", "EISDIR", "EINVAL", "ENFILE", "EMFILE", 
    /*  25 */ "ENOTTY", "ETXTBSY", "EFBIG", "ENOSPC", "ESPIPE", 
    /*  30 */ "EROFS", "EMLINK", "EPIPE", "EDOM", "ERANGE", 
    /*  35 */ "EDEADLK/EDEADLOCK", "ENAMETOOLONG", "ENOLCK", "ENOSYS", 
    /*  39 */ "ENOTEMPTY", "ELOOP", "", "ENOMSG", "EIDRM", "ECHRNG", 
    /*  45 */ "EL2NSYNC", "EL3HLT", "EL3RST", "ELNRNG", "EUNATCH", 
    /*  50 */ "ENOCSI", "EL2HLT", "EBADE", "EBADR", "EXFULL", "ENOANO", 
    /*  56 */ "EBADRQC", "EBADSLT", "", "EBFONT", "ENOSTR", "ENODATA", 
    /*  62 */ "ETIME", "ENOSR", "ENONET", "ENOPKG", "EREMOTE", 
    /*  67 */ "ENOLINK", "EADV", "ESRMNT", "ECOMM", "EPROTO", 
    /*  72 */ "EMULTIHOP", "EDOTDOT", "EBADMSG", "EOVERFLOW", 
    /*  76 */ "ENOTUNIQ", "EBADFD", "EREMCHG", "ELIBACC", "ELIBBAD", 
    /*  81 */ "ELIBSCN", "ELIBMAX", "ELIBEXEC", "EILSEQ", "ERESTART", 
    /*  86 */ "ESTRPIPE", "EUSERS", "ENOTSOCK", "EDESTADDRREQ", 
    /*  90 */ "EMSGSIZE", "EPROTOTYPE", "ENOPROTOOPT", 
    /*  93 */ "EPROTONOSUPPORT", "ESOCKTNOSUPPORT", 
    /*  95 */ "EOPNOTSUPP/ENOTSUP", "EPFNOSUPPORT", "EAFNOSUPPORT", 
    /*  98 */ "EADDRINUSE", "EADDRNOTAVAIL", "ENETDOWN", "ENETUNREACH", 
    /* 102 */ "ENETRESET", "ECONNABORTED", "ECONNRESET", "ENOBUFS", 
    /* 106 */ "EISCONN", "ENOTCONN", "ESHUTDOWN", "ETOOMANYREFS", 
    /* 110 */ "ETIMEDOUT", "ECONNREFUSED", "EHOSTDOWN", "EHOSTUNREACH", 
    /* 114 */ "EALREADY", "EINPROGRESS", "ESTALE", "EUCLEAN", 
    /* 118 */ "ENOTNAM", "ENAVAIL", "EISNAM", "EREMOTEIO", "EDQUOT", 
    /* 123 */ "ENOMEDIUM", "EMEDIUMTYPE", "ECANCELED", "ENOKEY", 
    /* 127 */ "EKEYEXPIRED", "EKEYREVOKED", "EKEYREJECTED", 
    /* 130 */ "EOWNERDEAD", "ENOTRECOVERABLE", "ERFKILL", "EHWPOISON"
};

#define MAX_ENAME 133


/**
 * 退出程序
 */
static void
terminate(bool use_exit)
{
    char *s;
    
    s = getenv("EF_DUMPCORE");

    // 生成coredump文件
    if (NULL != s && *s != '\0')
        abort();
    // 省去了对stdio缓冲区的刷新以及对退出处理程序的调用
    else if (use_exit)
        _exit(EXIT_FAILURE);
    // 正常的失败退出
    else
        exit(EXIT_FAILURE);
}

/*
    格式化打印错误信息
*/
static void
output_error(bool use_err, int err, bool flush_stdout, const char *format, va_list ap)
{
#define BUF_SIZE 500
    char buf[BUF_SIZE];
    char user_msg[BUF_SIZE];
    char err_text[BUF_SIZE];

    vsnprintf(user_msg, BUF_SIZE, format, ap);

    if (use_err)
        snprintf(err_text, BUF_SIZE, " [%s %s]", (err > 0 && err < MAX_ENAME) ? ename[err] : "?UNKOWN?", strerror(err));
    else
        snprintf(err_text, BUF_SIZE, ":");

    snprintf(buf, BUF_SIZE, "ERROR%s %s\n", err_text, user_msg);

    if (flush_stdout)
        fflush(stdout);
    fputs(buf, stderr);
    // 防止stderr不是行缓冲
    fflush(stderr);
}

/**
 * 
 */
void 
err_msg(const char *format, ...)
{
    va_list arg_list;
    int saved_errno;

    // 防止被改变
    saved_errno = errno;
    va_start(arg_list, format);
    output_error(true, errno, true, format, arg_list);
    va_end(arg_list);

    errno = saved_errno;
}

/**
 *  调用的是_exit，避免了对子进程继承自父进程的stdio缓冲区副本进行刷新，且不会调用父进程建立的退出程序
 */
void 
err_exit3(const char *format, ...)
{
    va_list arg_list;

    va_start(arg_list, format);
    output_error(true, errno, true, format, arg_list);
    va_end(arg_list);

    terminate(true);
}

/**
 *   调用的是exit
 */
void 
err_exit(const char *format, ...)
{
    va_list arg_list;

    va_start(arg_list, format);
    output_error(true, errno, false, format, arg_list);
    va_end(arg_list);

    terminate(false);
}

/**
 *   调用的是_exit，根据errnum打印字符串
 */
void 
err_exit_num(int errnum, const char *format, ...)
{
    va_list arg_list;

    va_start(arg_list, format);
    output_error(true, errnum, true, format, arg_list);
    va_end(arg_list);

    terminate(true);
}

/**
 * 与printf类似，增加退出机制
 */
void
fatal(const char *format, ...)
{
    va_list arg_list;

    va_start(arg_list, format);
    output_error(false, 0, true, format, arg_list);
    va_end(arg_list);

    terminate(true);
}

/**
 *  诊断命令行参数使用方面的错误，然后格式化输出
 */
void 
usage_err(const char *format, ...)
{
    va_list arg_list;

    fflush(stdout);

    fprintf(stderr, "Usage: ");
    // 获取可变参数列表的第一个参数的地址保存到arg_list
    va_start(arg_list, format);
    // 类似于fprintf，但适合参数可变列表传递
    vfprintf(stderr, format, arg_list);
    // 清空arg_list可变参数列表
    va_end(arg_list);

    fflush(stderr);
    exit(EXIT_FAILURE);
}

/**
 *   诊断特定程序的命令行参数
 */
void
cmdline_err(const char *format, ...)
{
    va_list arg_list;

    fflush(stdout);

    fprintf(stderr, "Command-line usage error: ");
    va_start(arg_list, format);
    vfprintf(stderr, format, arg_list);
    va_end(arg_list);

    fflush(stderr);
    exit(EXIT_FAILURE);
}
