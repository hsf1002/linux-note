
#include <errno.h>
#include "tty_func.h"


static struct termios *user_termios;

/*
    通用的信号处理函数
*/
static void
sig_handler(int signo)
{
    if (-1 == tcsetattr(STDIN_FILENO, TCSAFLUSH, &user_termios))
        perror("tcsetattr error");
    
    _exit(EXIT_SUCCESS);
}

/*
    处理SIGTSTP的信号处理函数
*/
static void
tstp_handler(int signo)
{
    struct termios our_termios;
    sigset_t tstp_mask;
    sigset_t prev_mask;
    struct sigaction sa;
    int save_errno;

    save_errno = errno;

    // 保存当前的终端属性
    if (-1 == tcgetattr(STDIN_FILENO, &our_termios))
        perror("tcgetattr error");
    // 设置当前的终端属性
    if (-1 == tcsetattr(STDIN_FILENO, TCSAFLUSH, &user_termios))
        perror("tcsetattr error");
    // 设置SIGTSTP的信号处置为默认动作
    if (SIG_ERR == signal(SIGTSTP, SIG_DFL))
        perror("signal SIGTSTP error");
    // 向自身进程发送SIGTSTP的信号
    raise(SIGTSTP);

    sigemptyset(&tstp_mask);
    sigaddset(&tstp_mask, SIGTSTP);
    // 取消阻塞信号SIGTSTP
    if (-1 == sigprocmask(SIG_UNBLOCK, &tstp_mask, &prev_mask))
        perror("sigprocmask error");

    //// 信号SIGCONT之后将在此恢复执行

    // 设置信号屏蔽字为最初状态
    if (-1 == sigprocmask(SIG_SETMASK, &prev_mask, NULL))
        perror("sigprocmask error");


    sigemptyset(&sa.sa_mask);
    sa.sa_mask = SA_RESTART;
    sa.sa_handler = tstp_handler;
    // 重新为信号SIGTSTP建立信号处理器
    if (-1 == sigaction(SIGTSTP, &sa, NULL))
        perror("sigaction error");

    // 保存当前的终端属性
    if (-1 == tcgetattr(STDIN_FILENO, &user_termios))
        perror("tcgetattr error");
    // 恢复当前的终端属性
    if (-1 == tcsetattr(STDIN_FILENO, TCSAFLUSH, &our_termios))
        perror("tcsetattr error");
    
    errno = save_errno;
}

/**
 * 
    演示 cbreak模式和演示模式
   
    原始模式
    ./test_tty_func
    skydeiMac:62-终端 sky$ stty
    speed 9600 baud;
    lflags: echoe echoke echoctl pendin
    oflags: -oxtabs
    cflags: cs8 -parenb

    skydeiMac:62-终端 sky$ ./test_tty_func
    abc // type adc, then Ctrl+J
    def // type DEF, then Ctrl+J, then Enter
    ^C^Z // Ctrl+C, Ctrl+Z, Ctrl+J
    qskydeiMac:62-终端 sky$  // 最后的q和终端提示符显示在同一行

    cbreak模式
    ./test_tty_func x
    xyz     // xyz, Ctrl+Z
    [1]+  Stopped                 ./test_tty_func x
    skydeiMac:62-终端 sky$ stty // 当前终端属性
    speed 9600 baud;
    lflags: echoe echoke echoctl pendin
    oflags: -oxtabs
    cflags: cs8 -parenb
    skydeiMac:62-终端 sky$ fg   // 恢复到前台      
    ./test_tty_func x
    ***     // 123, Ctrl+J
    skydeiMac:62-终端 sky$ stty // 再次查看当前终端属性，已经恢复
    speed 9600 baud;
    lflags: echoe echoke echoctl pendin
    oflags: -oxtabs
    cflags: cs8 -parenb
 */
int main(int argc, char *argv[])
{
    char ch;
    struct sigaction sa, prev;
    ssize_t n;

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    // cbreak模式
    if (argc > 1)
    {
        if (-1 == tty_set_cbreak(STDIN_FILENO, &user_termios))
            perror("tty_set_cbreak error");
        // 信号可以从终端产生，需要处理
        sa.sa_handler = sig_handler;

        if (-1 == sigaction(SIGQUIT, NULL, &prev))
            perror("sigaction error");
        // 如果信号SIGQUIT的默认处置不是忽略，则为其安装信号处理函数
        if (prev.sa_handler != SIG_IGN)
            if (-1 == sigaction(SIGQUIT, &sa, NULL))
                perror("sigaction error");
            
        if (-1 == sigaction(SIGINT, NULL, &prev))
            perror("sigaction error");
        // 如果信号SIGINT的默认处置不是忽略，则为其安装信号处理函数
        if (prev.sa_handler != SIG_IGN)
            if (-1 == sigaction(SIGINT, &sa, NULL))
                perror("sigaction error");

        sa.sa_handler = tstp_handler;
        if (-1 == sigaction(SIGTSTP, NULL, &prev))
            perror("sigaction error");
        // 如果信号SIGTSTP的默认处置不是忽略，则为其安装信号处理函数
        if (prev.sa_handler != SIG_IGN)
            if (-1 == sigaction(SIGTSTP, &sa, NULL))
                perror("sigaction error");
    }
    // 原始模式
    else
    {
        if (-1 == tty_set_raw(STDIN_FILENO, &user_termios))
            perror("tty_set_raw error");
    }

    sa.sa_handler = sig_handler;
    // 为信号SIGTERM安装信号处理函数，为了捕获KILL命令默认发送的信号
    if (-1 == sigaction(SIGTERM, &sa, &prev))
        perror("sigaction error");
    // 禁用标准输出缓冲
    setbuf(stdout, NULL);

    for(;;)
    {
        // 从标准输入读取一个字符
        n = read(STDIN_FILENO, &ch, 1);

        // 读取错误
        if (-1 == n)
        {
            perror("read error");
            break;
        }

        // 没有读到
        if (0 == n)
            break;
        // 字母则转小写
        if (isalpha((unsigned char)ch))
            putchar(tolower((unsigned char)ch));
        // 换行/回车，不做修改，直接回显
        else if (ch == '\n' || ch == '\r')
            putchar(ch);
        // 除了换行/回车外的控制字符，都以两个字符 Ctrl + X的形式回显 
        else if (iscntrl((unsigned char)ch))
            printf("^%c", ch ^ 64);
        // 其他字符都回显为*
        else
            putchar('*');
        // 如果是q，退出
        if (ch == 'q')
            break;
    }

    // 恢复当前的终端属性
    if (-1 == tcsetattr(STDIN_FILENO, TCSAFLUSH, &user_termios))
        perror("tcsetattr error");

    exit(EXIT_SUCCESS);
}

