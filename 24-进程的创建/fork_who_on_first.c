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
#include <sys/wait.h>

/**
 *   
 * 
./a.out 
parent: 0
child: 0
parent: 1
child: 1
parent: 2
child: 2
parent: 3
child: 3
parent: 4
child: 4
parent: 5
child: 5
parent: 6
child: 6
parent: 7
child: 7
parent: 8
child: 8
parent: 9
child: 9
parent: 10
child: 10
parent: 11
child: 11
parent: 12
child: 12
parent: 13
child: 13
parent: 14
child: 14
parent: 15
child: 15
parent: 16
child: 16
parent: 17
child: 17
parent: 18
child: 18
parent: 19
child: 19
parent: 20
child: 20
parent: 21
child: 21
parent: 22
child: 22
parent: 23
child: 23
parent: 24
child: 24
parent: 25
child: 25
parent: 26
child: 26
parent: 27
child: 27
parent: 28
child: 28
parent: 29
child: 29
parent: 30
child: 30
parent: 31
child: 31
parent: 32
child: 32
parent: 33
child: 33
parent: 34
child: 34
parent: 35
child: 35
parent: 36
child: 36
parent: 37
child: 37
parent: 38
child: 38
parent: 39
child: 39
parent: 40
child: 40
parent: 41
child: 41
parent: 42
child: 42
parent: 43
child: 43
parent: 44
child: 44
parent: 45
child: 45
parent: 46
child: 46
parent: 47
child: 47
parent: 48
child: 48
parent: 49
child: 49
parent: 50
child: 50
parent: 51
child: 51
parent: 52
child: 52
parent: 53
child: 53
parent: 54
child: 54
parent: 55
child: 55
parent: 56
child: 56
parent: 57
child: 57
parent: 58
child: 58
parent: 59
child: 59
parent: 60
child: 60
parent: 61
child: 61
parent: 62
child: 62
parent: 63
child: 63
parent: 64
child: 64
parent: 65
child: 65
parent: 66
child: 66
parent: 67
child: 67
parent: 68
child: 68
parent: 69
child: 69
parent: 70
child: 70
parent: 71
child: 71
parent: 72
child: 72
parent: 73
child: 73
parent: 74
child: 74
parent: 75
child: 75
parent: 76
child: 76
parent: 77
child: 77
parent: 78
child: 78
parent: 79
child: 79
parent: 80
child: 80
parent: 81
child: 81
parent: 82
child: 82
parent: 83
child: 83
parent: 84
child: 84
parent: 85
child: 85
parent: 86
child: 86
parent: 87
child: 87
parent: 88
child: 88
parent: 89
child: 89
parent: 90
child: 90
parent: 91
child: 91
parent: 92
child: 92
parent: 93
child: 93
parent: 94
child: 94
parent: 95
child: 95
parent: 96
child: 96
parent: 97
child: 97
parent: 98
child: 98
parent: 99
child: 99
 */
int
main(int argc, char *argv[])    
{
    int num_child = 100;
    pid_t child_pid;

    if (argc > 1 && 0 == strcmp(argv[1], "--help"))
        fprintf(stderr, "[num-child]\n", argv[0]);
    
    // num_child = (argc > 1) ? getInt(argv[1], GN_GT_0, "num-child") : 1;

    setbuf(stdout, NULL);

    for (int i=0; i<num_child; i++)
    {
        switch (child_pid = fork())
        {
        // error
        case -1:
            perror("fork error");
            break;
        // 子进程
        case 0:
            printf("child: %d\n", i);

            _exit(EXIT_SUCCESS);
        // 父进程
        default:
            
            printf("parent: %d\n", i);
            wait(NULL);

            break;
        }
    }

    exit(EXIT_SUCCESS);
}

