#include "argument.h"
#include "io.h"
#include "error.h"
#include "term.h"
#include "glconf.h"

#include <assert.h>
#include <stdio.h>
#include <unistd.h>

static void
run(const char *path)
{
}

static int
init(void)
{
        if (!get_terminal_xy(&glconf.term.w, &glconf.term.h))
                return 0;
        if (!enable_raw_terminal(STDIN_FILENO, &glconf.term.termios))
                return 0;
        term_fullscrn();

        return 1;
}

static void
cleanup(void)
{
        (void)disable_raw_terminal(STDIN_FILENO, &glconf.term.termios);
        term_exit_fullscrn();
}

int
main(int argc, char *argv[])
{
        char *path;

        path = parse_args(argc, argv);

        assert(!is_dir(path));

        if (!init())
                fatal("init");
        atexit(cleanup);

        run(path);

        return 0;
}
