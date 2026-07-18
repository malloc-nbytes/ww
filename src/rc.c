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

        int ok = 1;
        qcl_value *tabmode         = qcl_value_get(&config, "tabmode");
        qcl_value *show_trails     = qcl_value_get(&config, "show-trails");
        qcl_value *space_amt       = qcl_value_get(&config, "space-amt");
        qcl_value *compile_command = qcl_value_get(&config, "compile-command");
        qcl_value *to_clipboard    = qcl_value_get(&config, "to-clipboard");
        qcl_value *dumb_indent     = qcl_value_get(&config, "dumb-indent");
        qcl_value *artwork         = qcl_value_get(&config, "artwork");

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
        if (dumb_indent) {
                if (dumb_indent->kind != QCL_VALUE_KIND_BOOL) {
                        printf("wwrc error: dumbd-indent is expected to be a bool\n");
                        ok = 0;
                }
                else if (((qcl_value_bool *)dumb_indent)->b == 0)
                        glconf.flags |= FK_NODUMBINDENT;
        }
        if (artwork) {
                if (artwork->kind != QCL_VALUE_KIND_STRING) {
                        printf("wwrc error: artwork is expected to be a string\n");
                        ok = 0;
                }
                else
                        glconf.runtime.artwork = strdup(((qcl_value_string *)artwork)->s);
        }

        if (!ok) {
                anykey();
                return 0;
        }

        return 1;
}

