#include "argument.h"
#include "error.h"
#include "io.h"
#include "glconf.h"
#include "flags.h"

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

static void
parse_option1(const char *s)
{
        static void (*cmdfunc[])(void) = {
                usage,
                version,
        };

        static const char options[] = FLAG1CPL;

        _Static_assert(sizeof(cmdfunc)/sizeof(*cmdfunc) == sizeof(options)/sizeof(*options),
                       "commands and options must match");

        for (size_t i = 0; s[i]; ++i) {
                ssize_t func = -1;
                for (size_t j = 0; j < sizeof(options)/sizeof(*options); ++j) {
                        if (s[i] == options[j]) {
                                func = (ssize_t)j;
                                break;
                        }
                }
                if (func == -1)
                        fatal("unknown option `%c'", s[i]);
                cmdfunc[func]();
        }
}

static void
parse_option2(const char *s)
{
        ssize_t func;

        static void (*cmdfunc[])(void) = {
                usage,
                version,
        };

        static const char *options[] = FLAG2CPL;

        _Static_assert(sizeof(cmdfunc)/sizeof(*cmdfunc) == sizeof(options)/sizeof(*options),
                       "commands and options must match");

        func = -1;

        for (size_t i = 0; i < sizeof(options)/sizeof(*options); ++i) {
                if (!strcmp(s, options[i])) {
                        func = (ssize_t)i;
                        break;
                }
        }

        if (func == -1)
                fatal("unknown option `%s'", s);

        cmdfunc[func]();
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
                if (!it->h && it->s[0] == '+') {
                        int lineno = atoi(it->s+1);
                        if (!lineno && it->s[0] == '0')
                                fatal("invalid starting line number");
                        glconf.starting_lineno = lineno;
                }
                else if (it->h == 1) {
                        parse_option1(it->s);
                } else if (it->h == 2) {
                        parse_option2(it->s);
                } else if (filename)
                        fatal("only one filename is supported");
                else
                        filename = strdup(it->s);
                it       = it->n;
        }

        if (filename && is_dir(filename))
                fatal("given file must not be a directory");

        return filename;
}
