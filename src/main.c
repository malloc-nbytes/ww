#define QCL_IMPL
#include "qcl.h"
#include "window.h"
#include "buffer.h"
#include "argument.h"
#include "flags.h"
#include "error.h"
#include "term.h"
#include "io.h"
#include "glconf.h"
#include "utils.h"

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
        char   *real;
        int     file_provided;

        win           = window_create(glconf.term.w, glconf.term.h);
        real          = get_realpath(filename);
        file_provided = 1;

        if (real)
                fp = str_from(real);
        else if (!filename) {
                fp            = str_from("ww-help");
                file_provided = 0;
        }
        else
                fp = str_from(filename);

        if (!file_provided) {
                window_open_help_buffer(&win);
        } else {

                if (!(buffer = buffer_from_file(fp, &win)))
                        return 0;

                window_add_buffer(&win, buffer, 1);
                win.pb = win.ab;
                win.pbi = win.abi;
        }

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

static int
parse_config(void)
{
        //int space_amt;
        //const char *compile_cmd;

        qcl_config config = qcl_parse_file(glconf.config_filepath);
        if (!qcl_ok(&config)) {
                fprintf(stderr, "%s\n", qcl_geterr(&config));
                return 0;
        }

        int ok = 1;

        qcl_value *show_trails     = qcl_value_get(&config, "show-trails");
        qcl_value *spaces_are_tabs = qcl_value_get(&config, "spaces-are-tabs");
        qcl_value *space_amt       = qcl_value_get(&config, "space-amt");
        qcl_value *compile_command = qcl_value_get(&config, "compile-command");
        qcl_value *to_clipboard    = qcl_value_get(&config, "to-clipboard");

        if (show_trails && show_trails->kind != QCL_VALUE_KIND_BOOL) {
                printf("show-trails must be a boolean\n");
                ok = 0;
        }

        if (spaces_are_tabs && spaces_are_tabs->kind != QCL_VALUE_KIND_BOOL) {
                printf("spaces-are-tabs must be a boolean\n");
                ok = 0;
        }

        if (space_amt && space_amt->kind != QCL_VALUE_KIND_STRING) {
                printf("space-amt must be a number string\n");
                ok = 0;
        }
        if (space_amt && !cstr_isdigit(((qcl_value_string *)space_amt)->s)) {
                printf("space-amt must be a valid number string\n");
                ok = 0;
        }

        if (compile_command && compile_command->kind != QCL_VALUE_KIND_STRING) {
                printf("compile-command must be a string\n");
                ok = 0;
        }

        if (to_clipboard && to_clipboard->kind != QCL_VALUE_KIND_STRING) {
                printf("to-clipboard must be a string\n");
                ok = 0;
        }

        if (!ok)
                return 0;

        if (show_trails && ((qcl_value_bool *)show_trails)->b)
                glconf.flags |= FT_SHOWTRAILS;

        if (spaces_are_tabs && ((qcl_value_bool *)spaces_are_tabs)->b)
                glconf.flags |= FT_SPACESARETABS;

        if (space_amt)
                glconf.defaults.space_amt = atoi(((qcl_value_string *)space_amt)->s);

        if (compile_command)
                glconf.defaults.compile_cmd = ((qcl_value_string *)compile_command)->s;

        if (to_clipboard)
                glconf.defaults.to_clipboard = ((qcl_value_string *)to_clipboard)->s;

        return 1;
}

static int
setup_config_file(void)
{
        const char *home;

        if (!(home = gethome()))
                return 0;

        sprintf(glconf.config_filepath, "%s/%s", home, CONFIG_FILENAME);

        if (!file_exists(glconf.config_filepath)) {
                if (!create_file(glconf.config_filepath, 0))
                        perror("create_file");
                return 1;
        }

        if (!parse_config()) {
                anykey();
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

        if (!setup_config_file())
                fatal("aborting");

        if (!init())
                fatal("aborting");

        atexit(cleanup);
        filename = parse_args(argc, argv);
        int res = run(filename);
        free(filename);

        return res;
}
