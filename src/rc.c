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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wmissing-prototypes"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wenum-compare"
#pragma GCC diagnostic ignored "-Wformat-extra-args"
#pragma GCC diagnostic ignored "-Wunused-result"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wstringop-truncation"
#define QCL_IMPL
#include "qcl.h"
#pragma GCC diagnostic pop
#include "rc.h"
#include "io.h"
#include "term.h"
#include "utils.h"
#include "flags.h"
#include "colors.h"
#include "str.h"
#include "glconf.h"
#include "config.h"

#include <string.h>
#if HAVE_PATH_MAX
#include <limits.h>
#else
#define PATH_MAX 4096
#endif

const char *
get_config_path(void)
{
        static char buf[PATH_MAX] = {0};
        memset(buf, 0, sizeof(buf));
        const char *home = gethome();
        strcpy(buf, home);
        strcat(buf, "/.wwrc");
        return buf;
}

static char *
verify_colors(const char *c)
{
        str res = str_create();
        int ok = 1;
        char *buf = strdup(c);
        char *tok = strtok(buf, " ");
        static const char *accepted[] = {
                "yellow", "green", "bright_green", "gray",
                "red", "blue", "magenta", "orange", "white",
                "black", "cyan", "pink", "bright_pink", "purple",
                "bright_purple", "brown",

                "underline", "bold", "italic", "dim",
                "invert", "reset",
        };

        while (tok) {
                if (!strcmp(tok, "yellow"))             str_concat(&res, YELLOW);
                else if (!strcmp(tok, "green"))         str_concat(&res, GREEN);
                else if (!strcmp(tok, "bright_green"))  str_concat(&res, BRIGHT_GREEN);
                else if (!strcmp(tok, "gray"))          str_concat(&res, GRAY);
                else if (!strcmp(tok, "red"))           str_concat(&res, RED);
                else if (!strcmp(tok, "blue"))          str_concat(&res, BLUE);
                else if (!strcmp(tok, "magenta"))       str_concat(&res, MAGENTA);
                else if (!strcmp(tok, "orange"))        str_concat(&res, ORANGE);
                else if (!strcmp(tok, "white"))         str_concat(&res, WHITE);
                else if (!strcmp(tok, "black"))         str_concat(&res, BLACK);
                else if (!strcmp(tok, "cyan"))          str_concat(&res, CYAN);
                else if (!strcmp(tok, "pink"))          str_concat(&res, PINK);
                else if (!strcmp(tok, "bright_pink"))   str_concat(&res, BRIGHT_PINK);
                else if (!strcmp(tok, "purple"))        str_concat(&res, PURPLE);
                else if (!strcmp(tok, "bright_purple")) str_concat(&res, BRIGHT_PURPLE);
                else if (!strcmp(tok, "brown"))         str_concat(&res, BROWN);
                else if (!strcmp(tok, "underline"))     str_concat(&res, UNDERLINE);
                else if (!strcmp(tok, "bold"))          str_concat(&res, BOLD);
                else if (!strcmp(tok, "italic"))        str_concat(&res, ITALIC);
                else if (!strcmp(tok, "dim"))           str_concat(&res, DIM);
                else if (!strcmp(tok, "invert"))        str_concat(&res, INVERT);
                else if (!strcmp(tok, "reset"))         str_concat(&res, RESET);
                else {
                        printf("invalid color/effect `%s' in string `%s'\n", tok, c);
                        printf("valid values are:\n");
                        for (size_t i = 0; i < sizeof(accepted)/sizeof(*accepted); ++i)
                                printf("  %s\n", accepted[i]);
                        str_destroy(&res);
                        ok = 0;
                        break;
                }
                tok = strtok(NULL, " ");
        }

        free(buf);

        return ok ? res.chars : NULL;
}

int
parse_rc(void)
{
        const char *path;
        qcl_config config;

        path = get_config_path();

        if (!file_exists(path))
                return 1;

        config = qcl_parse_file(path);

        if (!qcl_ok(&config)) {
                printf("wwrc error: %s\n", qcl_geterr(&config));
                anykey();
                return 0;
        }

        int ok                         = 1;
        qcl_value *tabmode             = qcl_value_get(&config, "tab-mode");
        qcl_value *show_trails         = qcl_value_get(&config, "show-trails");
        qcl_value *space_amt           = qcl_value_get(&config, "space-amt");
        qcl_value *compile_command     = qcl_value_get(&config, "compile-command");
        qcl_value *to_clipboard        = qcl_value_get(&config, "to-clipboard");
        qcl_value *artwork             = qcl_value_get(&config, "artwork");
        qcl_value *selection_highlight = qcl_value_get(&config, "selection-highlight");

        if (tabmode) {
                if (tabmode->kind != QCL_VALUE_KIND_BOOL) {
                        printf("wwrc error: tabmode is expected to be a bool\n");
                        ok = 0;
                }
                else if (((qcl_value_bool *)tabmode)->b)
                        glconf.flags |= FK_TABMODE;
        }
        if (show_trails) {
                if (show_trails->kind != QCL_VALUE_KIND_BOOL) {
                        printf("wwrc error: show_trails is expected to be a bool\n");
                        ok = 0;
                }
                else if (((qcl_value_bool *)show_trails)->b)
                        glconf.flags |= FK_SHOWTRAILS;
        }
        if (space_amt) {
                if (space_amt->kind != QCL_VALUE_KIND_STRING) {
                        printf("wwrc error: space_amt is expected to be a string\n");
                        ok = 0;
                } else if (!cstr_isdigit(((qcl_value_string *)space_amt)->s)) {
                        ok = 0;
                        printf("wwrc error: space_amt must be a valid stringified integer\n");
                }
                else
                        glconf.runtime.space_amt = atoi(((qcl_value_string *)space_amt)->s);
        }
        if (compile_command) {
                if (compile_command->kind != QCL_VALUE_KIND_STRING) {
                        printf("wwrc error: compile_command is expected to be a string\n");
                        ok = 0;
                }
                else
                        glconf.runtime.compile = strdup(((qcl_value_string *)compile_command)->s);
        }
        if (to_clipboard) {
                if (to_clipboard->kind != QCL_VALUE_KIND_STRING) {
                        printf("wwrc error: to_clipboard is expected to be a string\n");
                        ok = 0;
                }
                else
                        glconf.runtime.to_clipboard = strdup(((qcl_value_string *)to_clipboard)->s);
        }
        if (artwork) {
                if (artwork->kind != QCL_VALUE_KIND_STRING) {
                        printf("wwrc error: artwork is expected to be a string\n");
                        ok = 0;
                }
                else
                        glconf.runtime.artwork = strdup(((qcl_value_string *)artwork)->s);
        }
        if (selection_highlight) {
                if (selection_highlight->kind != QCL_VALUE_KIND_STRING) {
                        printf("wwrc error: selection-highlight is expected to be a string\n");
                        ok = 0;
                } else {
                        char *c;
                        if (!(c = verify_colors(((qcl_value_string *)selection_highlight)->s)))
                                ok = 0;
                        else
                                glconf.runtime.selection_highlight = c;
                }
        }

        if (!ok) {
                anykey();
                return 0;
        }

        return 1;
}

