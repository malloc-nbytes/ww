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
void option_usage(argument **_);
int  parse_config_file(void);

#endif // ARGUMENT_H_INCLUDED
