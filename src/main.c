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
#include <unistd.h>

static void
run(const char *path)
{
        ww ed;

        ed = ww_create();

        ww_add_buffer(&ed, buffer_from(str_from(get_basename(path)),
                                       str_from(path),
                                       (unsigned)glconf.term.w, (unsigned)glconf.term.h,
                                       0, 0,
                                       lines_from(load_file(path))));

        ww_make_buffer_primary(&ed, 0);

        ww_run(&ed);
}

static int
init(void)
{
        if (!get_terminal_xy(&glconf.term.w, &glconf.term.h))
                return 0;
        if (!enable_raw_terminal(STDIN_FILENO, &glconf.term.termios))
                return 0;
        //term_fullscrn();
        clear_terminal();

        return 1;
}

static void
cleanup(void)
{
        (void)disable_raw_terminal(STDIN_FILENO, &glconf.term.termios);
        //term_exit_fullscrn();
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
