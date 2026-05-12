/*
 * ww: a simple editor
 * Copyright (C) 2026 malloc-nbytes
 * Contact: zdhdev@yahoo.com

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <https://www.gnu.org/licenses/>.
 */

#include "argument.h"
#include "error.h"
#include "io.h"
#include "colors.h"
#include "copying.h"
#include "config.h"
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
usage(void)
{
        printf("ww version " VERSION ", Copyright (C) 2026 malloc-nbytes\n");
        printf("ww comes with ABSOLUTELY NO WARRANTY.\n");
        printf("This is free software, and you are welcome to redistribute it\n");
        printf("under certain conditions; see option --copying\n\n");

        printf("Compilation Information:\n");
        printf("| cc: " COMPILER "\n");
        printf("| prefix: " PREFIX "\n\n");

        printf("Github repository: https://github.com/malloc-nbytes/ww.git\n\n");

        printf("Send bug reports to:\n");
        printf("  - https://github.com/malloc-nbytes/ww/issues\n");
        printf("  - or zdhdev@yahoo.com\n\n");

        printf("Usage: ww [OPTIONS...] [FILEPATH]\n");
        printf("Options:\n");
        printf("  -h, --help       show this information\n");
        printf("      --copying    show copying information\n");
        exit(0);
}

static void
copying(void)
{
        printf("%s", COPYING1);
        printf("%s", COPYING2);
        printf("%s", COPYING3);
        printf("%s", COPYING4);
        printf("%s", COPYING5);
        printf("%s", COPYING6);
        exit(0);
}

static void
parse_2hy_option(argument **a)
{
        const char *s = (*a)->s;

        if (!strcmp(s, FLAG_2HY_HELP))
                usage();
        if (!strcmp(s, FLAG_2HY_COPYING))
                copying();
        else
                fatal("unknown flag --%s", s);
}

static void
parse_1hy_option(argument **a)
{
        const char *s = (*a)->s;

        for (size_t i = 0; s[i]; ++i) {
                switch (s[i]) {
                case FLAG_1HY_HELP:
                        usage();
                        break;
                default:
                        fatal("unknown flag -%c", s[i]);
                }
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
                if (it->h == 1)
                        parse_1hy_option(&it);
                else if (it->h == 2)
                        parse_2hy_option(&it);
                else if (filename)
                        fatal("only one filename is supported");
                else if (file_exists(it->s))
                        filename = strdup(get_realpath(it->s));
                else
                        filename = strdup(it->s);
                it = it->n;
        }

        return filename;
}
