#ifndef ERROR_H_INCLUDED
#define ERROR_H_INCLUDED

#include <stdio.h>

#define fatal(msg, ...) \
        do {                                                            \
                fprintf(stderr, "fatal: " msg __VA_ARGS__ "\n");        \
                exit(1);                                                \
        } while (0)

#endif // ERROR_H_INCLUDED
