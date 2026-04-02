#include "flags.h"
#include "colors.h"
#include "line.h"

#include <stdint.h>
#include <stddef.h>
#include <termios.h>

struct {
        uint32_t    flags;
        char        config_filepath[512];
        size_t      starting_lineno;
        line_array  cfgvars;

        struct {
                int         space_amt;
                const char *compile_cmd;
                const char *to_clipboard;
                int         empty_line_squiggles;
                char        selection_highlight[128];
                const char *initial_buffers[32*sizeof(char *)];
                char        search_highlight[128];
                char        search_highlight_exact[128];
                char        menu_highlight[128];
                int         disable_quit_keybind;
                char       *ascii_art;
                int         enable_auto;
        } defaults;

        struct {
                size_t w;
                size_t h;
                struct termios old;
        } term;

        struct {
                char *path;
                char *query;
                char *repl;
        } replace;
} glconf = {
        .flags           = 0x0000,
        .config_filepath = {0},
        .starting_lineno = 0,
        .cfgvars         = {0},
        .defaults = {
                .space_amt              = DEFAULT_SPACE_AMT,
                .compile_cmd            = DEFAULT_COMPILE_COMMAND,
                .to_clipboard           = DEFAULT_TO_CLIPBOARD_COMMAND,
                .empty_line_squiggles   = DEFAULT_EMPTY_LINE_SQUIGGLES,
                .selection_highlight    = DEFAULT_SELECTION_HIGHLIGHT,
                .initial_buffers        = DEFAULT_INITIAL_BUFFERS,
                .search_highlight       = DEFAULT_SEARCH_HIGHLIGHT,
                .search_highlight_exact = DEFAULT_SEARCH_HIGHLIGHT_EXACT,
                .menu_highlight         = DEFAULT_MENU_HIGHLIGHT,
                .disable_quit_keybind   = DEFAULT_DISABLE_QUIT_KEYBIND,
                .ascii_art              = DEFAULT_ASCII_ART,
                .enable_auto            = DEFAULT_ENABLE_AUTOCOMPLETE,
        },
        .term = {
                .w   = 0,
                .h   = 0,
                .old = {0},
        },
        .replace = {
                .path  = NULL,
                .query = NULL,
                .repl  = NULL,
        },
};
