#define _POSIX_C_SOURCE 199309
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
#include <setjmp.h>
#include "get_num.h"


/**
 *   
 * 使用sigqueue发送实时信号
 * 
 cc t_sigqueue.c -o t_sigqueue libgetnum.so

./sig_receiver 15
./sig_receiver: PID is 1430440 
after fillset mask: 
		1 (Hangup)
		2 (Interrupt)
		3 (Quit)
		4 (Illegal instruction)
		5 (Trace/breakpoint trap)
		6 (Aborted)
		7 (Bus error)
		8 (Floating point exception)
		9 (Killed)
		10 (User defined signal 1)
		11 (Segmentation fault)
		12 (User defined signal 2)
		13 (Broken pipe)
		14 (Alarm clock)
		15 (Terminated)
		16 (Stack fault)
		17 (Child exited)
		18 (Continued)
		19 (Stopped (signal))
		20 (Stopped)
		21 (Stopped (tty input))
		22 (Stopped (tty output))
		23 (Urgent I/O condition)
		24 (CPU time limit exceeded)
		25 (File size limit exceeded)
		26 (Virtual timer expired)
		27 (Profiling timer expired)
		28 (Window changed)
		29 (I/O possible)
		30 (Power failure)
		31 (Bad system call)
		34 (Real-time signal 0)
		35 (Real-time signal 1)
		36 (Real-time signal 2)
		37 (Real-time signal 3)
		38 (Real-time signal 4)
		39 (Real-time signal 5)
		40 (Real-time signal 6)
		41 (Real-time signal 7)
		42 (Real-time signal 8)
		43 (Real-time signal 9)
		44 (Real-time signal 10)
		45 (Real-time signal 11)
		46 (Real-time signal 12)
		47 (Real-time signal 13)
		48 (Real-time signal 14)
		49 (Real-time signal 15)
		50 (Real-time signal 16)
		51 (Real-time signal 17)
		52 (Real-time signal 18)
		53 (Real-time signal 19)
		54 (Real-time signal 20)
		55 (Real-time signal 21)
		56 (Real-time signal 22)
		57 (Real-time signal 23)
		58 (Real-time signal 24)
		59 (Real-time signal 25)
		60 (Real-time signal 26)
		61 (Real-time signal 27)
		62 (Real-time signal 28)
		63 (Real-time signal 29)
		64 (Real-time signal 30)
./sig_receiver: sleeping for 15 seconds
./sig_receiver: pending signals are: 
		28 (Window changed)
after emptyset mask: 
		<empty signal set>
^\^\^C./sig_receiver: signal 3 caught 2 times (SIGQUIT)手动按键Ctrl + \两次
./sig_receiver: signal 10 caught 8 times      (SIGUSR1)手动发送信号10 十次
./sig_receiver: signal 28 caught 3 times      (SIGWINCH)焦点切换

./t_sigqueue 1430440 10 888 10

 */
int
main(int argc, char *argv[])    
{
    int sig_no;
    int num_sig;
    int sig_data;
    union sigval sv;

    if (argc < 4 || 0 == strcmp(argv[1], "--help"))
        fprintf(stderr, "%s pid sig_no data [num-sigs]\n", argv[0]);
    
    printf("%s PID: %ld, UID: %ld\n ", argv[0], (long)getpid(), (long)getuid());

    sig_no = getInt(argv[2], 0, "sig-num");
    sig_data = getInt(argv[3], GN_ANY_BASE, "data");
    num_sig = (argc > 4) ? getInt(argv[4], GN_GT_0, "num_sig") : 1;

    for (int i=0; i<num_sig; i++)
    {
        sv.sival_int = sig_data + 1;
        if (-1 == sigqueue(getLong(argv[1], 0, "pid"), sig_no, sv))
            perror("sigqueue error");
    }

    exit(EXIT_SUCCESS);
}

