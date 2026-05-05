#include "argument.h"
#include "io.h"
#include "error.h"
#include "term.h"
#include "buffer.h"
#include "line.h"
#include "ww.h"
#include "glconf.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#define _GNU_SOURCE
#include <unistd.h>

static void
sigint_handler(int sig)
{
        (void)sig;
}

static void
run(const char *path)
{
        ww ed;

        ed = ww_create();

        if (!file_exists(path))
                create_file(path, 1);

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

        term_fullscrn();
        clear_terminal();
        //enable_mousewheel_capture();

        return 1;
}

static void
cleanup(void)
{
        (void)disable_raw_terminal(STDIN_FILENO, &glconf.term.termios);
        term_exit_fullscrn();
        //disable_mousewheel_capture();
}

int
main(int argc, char *argv[])
{
        char *path;

        path = parse_args(argc, argv);

        /* assert(!is_dir(path)); */

        if (!init())
                fatal("init");
        atexit(cleanup);

        init_buffer_translation_unit();

        run(path);

        return 0;
}
