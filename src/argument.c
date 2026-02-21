#include "argument.h"
#include "error.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static char *
argument_eat(int *argc, char ***argv)
{
        if (*argc <= 0)
                return NULL;
        --(*argc);
        char *cur = **argv;
        (*argv)++;
        return cur;
}

argument *
argument_alloc(int    argc,
               char **argv,
               int    skip_first)
{
        if (skip_first)
                --argc, ++argv;

        argument *hd = NULL, *tl = NULL;

        while (argc > 0) {
                argument *it = (argument *)malloc(sizeof(argument));
                it->s = NULL;
                it->h = 0;
                it->eq = NULL;
                it->n = NULL;

                char *arg = strdup(argument_eat(&argc, &argv));

                if (arg[0] == '-' && arg[1] && arg[1] == '-') {
                        it->h = 2;
                        arg += 2;
                } else if (arg[0] == '-') {
                        it->h = 1;
                        ++arg;
                } else {
                        it->h = 0;
                }

                for (size_t i = 0; arg[i]; ++i) {
                        if (arg[i] == '=') {
                                arg[i] = '\0';
                                it->eq = strdup(arg+i+1);
                                break;
                        }
                }

                it->s = strdup(arg);

                if (!hd && !tl) {
                        hd = tl = it;
                } else {
                        argument *tmp = tl;
                        tl = it;
                        tmp->n = tl;
                }
        }

        return hd;
}

void
argument_free(argument *arg)
{
        while (arg) {
                free(arg->s);
                free(arg->eq);
                argument *tmp = arg;
                arg = arg->n;
                free(tmp);
        }
}

char *
parse_args(int argc, char *argv[])
{
        char     *filename;
        argument *hd;
        argument *it;

        filename = NULL;
        hd       = argument_alloc(argc, argv, 1);
        it       = hd;

        while (it) {
                if (it->h)
                        fatal("options are unimplemented");
                if (filename)
                        fatal("only one filename is supported");
                filename = strdup(it->s);
                it       = it->n;
        }

        return filename;
}
