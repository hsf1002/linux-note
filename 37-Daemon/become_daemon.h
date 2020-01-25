#ifndef BECOME_DAEMON_H             
#define BECOME_DAEMON_H


// 不改变根目录
#define BD_NO_CHDIR           01    
// 不关闭继承的文件描述符
#define BD_NO_CLOSE_FILES     02    
// 不重新打开标准输入/标准输出，并将标准错误定向到/dev/null
#define BD_NO_REOPEN_STD_FDS  04    
// 不重新设置文件模式屏蔽字
#define BD_NO_UMASK0         010    

// 如果sysconf(_SC_OPEN_MAX)是确定的，需要关闭的文件描述符最大值
#define BD_MAX_CLOSE  8192          

int become_daemon(int flags);

#endif
