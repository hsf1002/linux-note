#ifndef PRINT_RUSAGE_H      /* Prevent accidental double inclusion */
#define PRINT_RUSAGE_H

#include <sys/resource.h>

void print_usage(const char *leader, const struct rusage *ru);

#endif
