#include "argument.h"
#include "error.h"
#include "io.h"
#include "glconf.h"
#include "flags.h"
#include "colors.h"
#include "config.h"
#include "copying.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

void
option_usage(argument **_)
{
        (void)_;

        printf("ww version 1.0, Copyright (C) 2025 malloc-nbytes.\n");
        printf("ww comes with ABSOLUTELY NO WARRANTY.\n");
        printf("This is free software, and you are welcome to redistribute it\n");
        printf("under certain conditions; see command `copying`.\n\n");

        printf("Compiler information:\n");
        printf("| cc: " COMPILER_NAME "\n");
        printf("| path: " COMPILER_PATH "\n");
        printf("| ver.: " COMPILER_VERSION "\n\n");

        printf("Usage: ww " YELLOW "[OPTIONS...]" RESET " " GREEN BOLD "<FILEPATH>" RESET GRAY " [+LINENO]" RESET "\n\n");
        printf("Options:\n");
        printf("  " YELLOW BOLD "-h, --help" "            " RESET "     display this message\n");
        printf("  " YELLOW BOLD "-v, --version" "         " RESET "     show the version\n");
        printf("  " YELLOW BOLD "    --copying" "         " RESET "     show copying information\n");
        printf("  " YELLOW BOLD "    --replace query=<str>\n");
        printf("                repl=<str>  " RESET " find and replace `query' with `repl'\n");
        exit(0);
}

static void
option_copying(argument **_)
{
        (void)_;
        printf(COPYING1);
        printf(COPYING2);
        printf(COPYING3);
        printf(COPYING4);
        printf(COPYING5);
        printf(COPYING6);
        exit(0);
}

static void
option_version(argument **_)
{
        (void)_;

        printf("ww-v" VERSION "\n");
        exit(0);
}

static void
option_replace(argument **a)
{
        char *query;
        char *repl;

        *a = (*a)->n;

        if (!*a || !(*a)->n)
                goto bad;

        if (strcmp((*a)->s, "query"))
                goto bad;

        query = strdup((*a)->eq);

        *a = (*a)->n;

        if (strcmp((*a)->s, "repl"))
                goto bad;

        repl = strdup((*a)->eq);

        glconf.flags         |= FT_REPLACE;
        glconf.replace.query  = query;
        glconf.replace.repl   = repl;

        return;
 bad:
        fatal("incorrect usage for option `replace', see --help");
}

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
parse_option1(argument **a)
{
        const char *s;

        static void (*cmdfunc[])(argument **) = {
                option_usage,
                option_version,
        };

        static const char options[] = FLAG1CPL;

        _Static_assert(sizeof(cmdfunc)/sizeof(*cmdfunc) == sizeof(options)/sizeof(*options),
                       "commands and options must match");

        s = (*a)->s;

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
                cmdfunc[func](a);
        }
}

static void
parse_option2(argument **a)
{
        ssize_t func;
        const char *s;

        static void (*cmdfunc[])(argument **) = {
                option_usage,
                option_version,
                option_copying,
                option_replace,
        };

        static const char *options[] = FLAG2CPL;

        _Static_assert(sizeof(cmdfunc)/sizeof(*cmdfunc) == sizeof(options)/sizeof(*options),
                       "commands and options must match");

        func = -1;

        s = (*a)->s;

        for (size_t i = 0; i < sizeof(options)/sizeof(*options); ++i) {
                if (!strcmp(s, options[i])) {
                        func = (ssize_t)i;
                        break;
                }
        }

        if (func == -1)
                fatal("unknown option `%s'", s);

        cmdfunc[func](a);
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
                        parse_option1(&it);
                } else if (it->h == 2) {
                        parse_option2(&it);
                } else if (filename)
                        fatal("only one filename is supported");
                else
                        filename = strdup(it->s);
                it = it->n;
        }

        if (glconf.flags & FT_REPLACE) {
                if (!filename)
                        fatal("option `far' requires a file or directory");
                glconf.replace.path = filename;
        }

        return filename;
}
