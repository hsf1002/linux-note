#ifndef CREATE_PID_FILE_H   
#define CREATE_PID_FILE_H

#define CPE_CLOEXEC 1

create_pid_file(const char* prog_name, const char *pid_file, int flag);

#endif
