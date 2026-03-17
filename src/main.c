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
#include "lexer.h"
#include "colors.h"
#include "calc.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#define _GNU_SOURCE
#include <unistd.h>

static void
open_initial_buffers(window *win)
{
        const char **def;
        int          added;

        def   = glconf.defaults.initial_buffers;
        added = 0;

        for (size_t i = 0; def[i]; ++i) {
                if (!strcmp(def[i], "ww-help")) {
                        window_open_help_buffer(win);
                        added = 1;
                } else if (!strcmp(def[i], "ww-calc")) {
                        window_open_calc_buffer(win);
                        added = 1;
                } else if (!strcmp(def[i], "ww-compilation")) {
                        window_compilation_buffer(win);
                        added = 1;
                }
        }

        if (!added)
                window_open_help_buffer(win);
}

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

        if (real && is_dir(real)) {
                if (chdir(real) != 0)
                        fatal("could not chdir to `%s'", real);
                fp            = str_from("ww-help");
                file_provided = 0;
        }
        else if (real)
                fp = str_from(real);
        else
                fp = str_from(filename);

        if (!file_provided) {
                open_initial_buffers(&win);
        } else {

                if (!(buffer = buffer_from_file(fp, &win)))
                        return 0;

                buffer->al = glconf.starting_lineno ? glconf.starting_lineno-1 : 0;
                buffer->cy = buffer->al;
                window_add_buffer(&win, buffer, 1);
                win.pb = win.ab;
                win.pbi = win.abi;
        }

        init_buffer_context();
        calc_init();

        clear_terminal();
        window_handle(&win);

        return 1;
}

static int
replace_in_file(const char *path)
{
        // NOTE: returns 1 on failure, 0 on success.

        assert(*glconf.replace.query);

        char       *data;
        const char *query;
        size_t      query_n;
        const char *repl;
        str         newd;
        int         changed;
        int         res;

        if (!(data = load_file(path)))
                return 1;

        query   = glconf.replace.query;
        query_n = strlen(query);
        repl    = glconf.replace.repl;
        newd    = str_create();
        changed = 0;
        res     = 0;

        for (size_t i = 0, lineno = 1; data[i]; ++i) {
                if (!memcmp(query, data+i, query_n)) {
                        changed = 1;

                        str_concat(&newd, repl);
                        i += query_n-1;

                        printf("F %s %zu: %s", path, lineno, repl);
                        for (size_t j = i+1; data[j] && data[j] != '\n'; ++j)
                                putchar(data[j]);
                        putchar('\n');
                }
                else
                        str_append(&newd, data[i]);

                if (data[i] == '\n')
                        ++lineno;
        }

        if (changed)
                if (!write_file(path, str_cstr(&newd)))
                        res = 1;

        free(data);
        str_destroy(&newd);

        return res;
}

static int
replace_in_dir(const char *dir)
{
        // NOTE: returns 1 on failure, 0 on success.

        cstr_array entries;
        int        res;

        entries = lsdir(dir);
        res     = 0;

        printf("D %s\n", dir);

        for (size_t i = 0; i < entries.len; ++i) {
                const char *e;
                str         realpath;
                const char *rpraw;

                e = entries.data[i];

                if (strcmp(e, ".")==0 || strcmp(e, "..")==0)
                        continue;

                realpath = str_from(dir);
                if (str_at(&realpath, realpath.len-1) != '/')
                        str_append(&realpath, '/');
                str_concat(&realpath, e);

                rpraw = str_cstr(&realpath);

                if (is_dir(rpraw)) {
                        if (replace_in_dir(rpraw) != 0) {
                                res = 1;
                                goto done;
                        }
                } else {
                        if (replace_in_file(rpraw) != 0) {
                                res = 1;
                                goto done;
                        }
                }
        }

        dyn_array_free(entries);

done:
        return res;
}

static int
run_replace_option(void)
{
        // NOTE: returns 1 on failure, 0 on success.

        const char *path;

        path = glconf.replace.path;

        if (!file_exists(path))
                return 1;
        return is_dir(path)
                ? replace_in_dir(path)
                : replace_in_file(path);
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

static const char *
parse_colors_from_config(const char *s)
{
        char *tok;
        static char buf[128] = {0};

        tok = strtok(s, " ");

        while (tok) {
                if (strlen(buf) > 128) {
                        printf("selection-highlight exceeds 128 bytes\n");
                        return NULL;
                }

                if (!strcmp(tok, "yellow"))
                        strcat(buf, YELLOW);
                else if (!strcmp(tok, "green"))
                        strcat(buf, GREEN);
                else if (!strcmp(tok, "bright_green"))
                        strcat(buf, BRIGHT_GREEN);
                else if (!strcmp(tok, "gray"))
                        strcat(buf, GRAY);
                else if (!strcmp(tok, "red"))
                        strcat(buf, RED);
                else if (!strcmp(tok, "blue"))
                        strcat(buf, BLUE);
                else if (!strcmp(tok, "cyan"))
                        strcat(buf, CYAN);
                else if (!strcmp(tok, "magenta"))
                        strcat(buf, MAGENTA);
                else if (!strcmp(tok, "orange"))
                        strcat(buf, ORANGE);
                else if (!strcmp(tok, "white"))
                        strcat(buf, WHITE);
                else if (!strcmp(tok, "black"))
                        strcat(buf, BLACK);
                else if (!strcmp(tok, "pink"))
                        strcat(buf, PINK);
                else if (!strcmp(tok, "bright_pink"))
                        strcat(buf, BRIGHT_PINK);
                else if (!strcmp(tok, "purple"))
                        strcat(buf, PURPLE);
                else if (!strcmp(tok, "bright_purple"))
                        strcat(buf, BRIGHT_PURPLE);
                else if (!strcmp(tok, "brown"))
                        strcat(buf, BROWN);
                else if (!strcmp(tok, "underline"))
                        strcat(buf, UNDERLINE);
                else if (!strcmp(tok, "bold"))
                        strcat(buf, BOLD);
                else if (!strcmp(tok, "italic"))
                        strcat(buf, ITALIC);
                else if (!strcmp(tok, "dim"))
                        strcat(buf, DIM);
                else if (!strcmp(tok, "invert"))
                        strcat(buf, INVERT);
                else if (!strcmp(tok, "reset"))
                        strcat(buf, RESET);
                else {
                        printf("invalid selection-highlight: `%s'\n", tok);
                        return NULL;
                }

                tok = strtok(NULL, " ");
        }

        return buf;
}

static int
set_default_color(const char       *dst,
                  qcl_value_string *var)
{
        char *colors;
        memset(dst, 0, sizeof(dst));
        if (!(colors = parse_colors_from_config(var->s)))
                return 0;
        else
                strcpy(dst, colors);
        return 1;
}

static int
parse_config(void)
{
        qcl_config config = qcl_parse_file(glconf.config_filepath);
        if (!qcl_ok(&config)) {
                fprintf(stderr, "%s\n", qcl_geterr(&config));
                return 0;
        }

        int ok = 1;

        qcl_value *show_trails            = qcl_value_get(&config, "show-trails");
        qcl_value *tabmode                = qcl_value_get(&config, "tab-mode");
        qcl_value *space_amt              = qcl_value_get(&config, "space-amt");
        qcl_value *compile_command        = qcl_value_get(&config, "compile-command");
        qcl_value *to_clipboard           = qcl_value_get(&config, "to-clipboard");
        qcl_value *line_squiggles         = qcl_value_get(&config, "empty-line-squiggles");
        qcl_value *selection_highlight    = qcl_value_get(&config, "selection-highlight");
        qcl_value *initial_buffers        = qcl_value_get(&config, "initial-buffers");
        qcl_value *search_highlight       = qcl_value_get(&config, "search-highlight");
        qcl_value *search_highlight_exact = qcl_value_get(&config, "search-highlight-exact");
        qcl_value *menu_highlight         = qcl_value_get(&config, "menu-highlight");

        if (show_trails && show_trails->kind != QCL_VALUE_KIND_BOOL) {
                printf("show-trails must be a boolean\n");
                ok = 0;
        }
        if (tabmode && tabmode->kind != QCL_VALUE_KIND_BOOL) {
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
        if (line_squiggles && line_squiggles->kind != QCL_VALUE_KIND_BOOL) {
                printf("empty-line-squiggles must be a boolean\n");
                ok = 0;
        }
        if (selection_highlight && selection_highlight->kind != QCL_VALUE_KIND_STRING) {
                printf("selection-highlight must be a string\n");
                ok = 0;
        }
        if (initial_buffers && initial_buffers->kind != QCL_VALUE_KIND_LIST) {
                printf("initial-buffers must be a list of strings\n");
                ok = 0;
        }
        if (search_highlight && search_highlight->kind != QCL_VALUE_KIND_STRING) {
                printf("search-highlight must be a string\n");
                ok = 0;
        }
        if (search_highlight_exact && search_highlight_exact->kind != QCL_VALUE_KIND_STRING) {
                printf("search-highlight-exact must be a string\n");
                ok = 0;
        }
        if (menu_highlight && menu_highlight->kind != QCL_VALUE_KIND_STRING) {
                printf("menu-highlight must be a string\n");
                ok = 0;
        }

        int res = 1;

        if (!ok)
                return 0;

        if (show_trails && ((qcl_value_bool *)show_trails)->b)
                glconf.flags |= FT_SHOWTRAILS;

        if (tabmode && ((qcl_value_bool *)tabmode)->b)
                glconf.flags |= FT_TABMODE;

        if (space_amt)
                glconf.defaults.space_amt = atoi(((qcl_value_string *)space_amt)->s);

        if (compile_command)
                glconf.defaults.compile_cmd = ((qcl_value_string *)compile_command)->s;

        if (to_clipboard)
                glconf.defaults.to_clipboard = ((qcl_value_string *)to_clipboard)->s;

        if (line_squiggles)
                glconf.defaults.empty_line_squiggles = ((qcl_value_bool *)line_squiggles)->b;

        if (selection_highlight) {
                if (!set_default_color(glconf.defaults.selection_highlight, (qcl_value_string *)selection_highlight))
                        res = 0;
        }

        if (initial_buffers) {
                char **lst = qcl_value_flatten(&config, "initial-buffers");
                size_t i;
                for (i = 0; lst[i]; ++i)
                        glconf.defaults.initial_buffers[i] = lst[i];
                glconf.defaults.initial_buffers[i] = NULL;
                free(lst);
        }

        if (search_highlight) {
                if (!set_default_color(glconf.defaults.search_highlight, (qcl_value_string *)search_highlight))
                        res = 0;
        }

        if (search_highlight_exact) {
                if (!set_default_color(glconf.defaults.search_highlight_exact, (qcl_value_string *)search_highlight_exact))
                        res = 0;
        }

        if (menu_highlight) {
                if (!set_default_color(glconf.defaults.menu_highlight, (qcl_value_string *)menu_highlight))
                        res = 0;
        }

        return res;
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

        if (!(filename = parse_args(argc, argv)))
            option_usage(NULL);

        if (glconf.flags & FT_REPLACE)
                return run_replace_option();

        if (!init())
                fatal("aborting");
        atexit(cleanup);
        int res = run(filename);
        free(filename);
        return res;
}
