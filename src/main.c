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
#include "io.h"
#include "error.h"
#include "term.h"
#include "buffer.h"
#include "line.h"
#include "ww.h"
#include "rc.h"
#include "glconf.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#define _GNU_SOURCE
#include <unistd.h>

static void
sigint_handler(int sig) { (void)sig; }

static void
run(const char *path)
{
        ww ed;

        ed = ww_create();

        if (!file_exists(path)) {
                (void)create_file(path, 1);
                (void)write_file(path, "\n");
        }

        if (path && !is_dir(path)) {
                ww_add_buffer(&ed, buffer_from(str_from(get_basename(path)),
                                               str_from(path),
                                               (unsigned)glconf.term.w, (unsigned)glconf.term.h,
                                               0, 0,
                                               lines_from(load_file(path)), &ed));
        } else {
                if (path) {
                        if (chdir(path) != 0) {
                                perror("chdir");
                                fatal("aborting");
                        }
                }
                ww_add_buffer(&ed, ww_helpbuf_alloc((unsigned)glconf.term.w,
                                                    (unsigned)glconf.term.h, 0, 0, &ed));
        }

        ww_make_buffer_primary(&ed, 0);

        ww_run(&ed);
}

static int
init(void)
{
        (void)parse_rc();

        if (!glconf.runtime.compile)
                glconf.runtime.compile = strdup("make");

        struct sigaction sa;
        sa.sa_handler = sigint_handler;
        sa.sa_flags = 0;
        sigemptyset(&sa.sa_mask);

        if (sigaction(SIGINT, &sa, NULL) == 1) {
                perror("sigaction");
                return 0;
        }

        if (!get_terminal_xy(&glconf.term.w, &glconf.term.h))
                return 0;
        if (!enable_raw_terminal(STDIN_FILENO, &glconf.term.termios))
                return 0;

        enable_bracketed_paste();
        term_fullscrn();
        clear_terminal();
        //enable_mousewheel_capture();

        return 1;
}

static void
cleanup(void)
{
        (void)disable_raw_terminal(STDIN_FILENO, &glconf.term.termios);
        disable_bracketed_paste();
        term_exit_fullscrn();
        //disable_mousewheel_capture();
}

int
main(int argc, char *argv[])
{
        char *path;

        path = parse_args(argc, argv);

        if (!init())
                fatal("init");
        atexit(cleanup);

        init_buffer_translation_unit();

        run(path);

        return 0;
}
