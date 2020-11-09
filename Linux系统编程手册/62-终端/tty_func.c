
#include "tty_func.h"

/**
 * 设置为cbreak模式
 */
int
tty_set_cbreak(int fd, struct termios *prev_termios)
{
    struct termios t;

    // 设置终端属性
    if (-1 == tcgetattr(fd, &t))
        return -1;
    // 保存终端属性
    if (NULL != prev_termios)
        *prev_termios = t;
    
    // 关闭ICANON和回显标记
    t.c_lflag &= ~(ICANON | ECHO);
    // 产生信号的字符会被解释
    t.c_lflag |= ISIG;

    // 输入时将CR映射为NL
    t.c_iflag &= ~ICRNL;
    // 最小字符数量为1个字节
    t.c_cc[VMIN] = 1;
    // 最小时间数量为0
    t.c_cc[VTIME] = 0;

    // 设置终端属性
    if (-1 == tcsetattr(fd, TCSAFLUSH, &t))
        return -1;
    
    return 0;
} 

/**
 * 设置为原始模式
 */
int
tty_set_raw(int fd, struct termios *prev_termios)
{
    struct termios t;

    // 设置终端属性
    if (-1 == tcgetattr(fd, &t))
        return -1;
    // 保存终端属性
    if (NULL != prev_termios)
        *prev_termios = t;
    
    // 关闭ICANON，回显标记，产生信号的字符不会被解释，关闭对输入字符的扩展处理
    t.c_lflag &= ~(ICANON | ECHO | ISIG | IEXTEN);

    // 在break状态下发出信号中断SIGINT，输入时将CR映射为NL，忽略break状态，输入中忽略CR，输入中将NL映射为CR
    // 开启输入奇偶校验检查，输入字符中去掉最高位，开启开始/停止输出流控，标记奇偶校验错误
    t.c_iflag &= ~(BRKINT | ICRNL | IGNBRK | IGNCR | INLCR | INPCK | ISTRIP | IXON | PARMRK);

    // 关闭输出后续处理
    t.c_oflag &= ~OPOST;

    // 最小字符数量为1个字节
    t.c_cc[VMIN] = 1;
    // 最小时间数量为0
    t.c_cc[VTIME] = 0;

    // 设置终端属性
    if (-1 == tcsetattr(fd, TCSAFLUSH, &t))
        return -1;
    
    return 0;
} 

