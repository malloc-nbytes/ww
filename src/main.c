#include "window.h"
#include "buffer.h"
#include "argument.h"
#include "flags.h"
#include "error.h"
#include "term.h"
#include "glconf.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int
run(const char *filename)
{
        window  win;
        buffer *buffer;
        str     fp;

        fp     = str_from(filename);
        win    = window_create(glconf.term.w, glconf.term.h);

        if (!(buffer = buffer_from_file(fp, &win)))
                return 0;

        window_add_buffer(&win, buffer, 1);
        win.pb = win.ab;
        win.pbi = win.abi;
        window_handle(&win);

        return 1;
}

static void
sigint_handler(int sig)
{
        (void)sig;
}

static int
init(void)
{
        if (!enable_raw_terminal(STDIN_FILENO, &glconf.term.old)) {
                perror("enable_raw_terminal");
                return 0;
        }

        if (!get_terminal_xy(&glconf.term.w, &glconf.term.h)) {
                perror("get_terminal_xy");
                return 0;
        }

        struct sigaction sa;
        sa.sa_handler = sigint_handler;
        sa.sa_flags = 0;
        sigemptyset(&sa.sa_mask);

        if (sigaction(SIGINT, &sa, NULL) == 1) {
                perror("sigaction");
                return 0;
        }

        return 1;
}

static void
cleanup(void)
{
        clear_terminal();
        if (!disable_raw_terminal(STDIN_FILENO, &glconf.term.old))
                perror("disable_raw_terminal");
}

int
main(int argc, char *argv[])
{
        char *filename;

        if (argc <= 1)
                usage();

        if (!init())
                fatal("aborting");

        atexit(cleanup);
        filename = parse_args(argc, argv);
        int res = run(filename);
        free(filename);

        return res;
}
