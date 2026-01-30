#ifndef ARGUMENT_H_INCLUDED
#define ARGUMENT_H_INCLUDED

#include <stddef.h>

typedef struct argument {
        char             *s;
        size_t            h;
        char             *eq;
        struct argument *n;
} argument;

char *parse_args(int argc, char *argv[]);
argument *argument_alloc(int    argc,
                         char **argv,
                         int    skip_first);
void argument_free(argument *arg);

#endif // ARGUMENT_H_INCLUDED
