#ifndef GLCONF_H_INCLUDED
#define GLCONF_H_INCLUDED

#include "line.h"

#include <stddef.h>
#include <stdint.h>
#include <termios.h>

#define CFGVAR_SHOW_TRAILS            "show-trails"
#define CFGVAR_TAB_MODE               "tab-mode"
#define CFGVAR_SPACE_AMT              "space-amt"
#define CFGVAR_COMPILE_COMMAND        "compile-command"
#define CFGVAR_TO_CLIPBOARD           "to-clipboard"
#define CFGVAR_EMPTY_LINE_SQUIGGLES   "empty-line-squiggles"
#define CFGVAR_SELECTION_HIGHLIGHT    "selection-highlight"
#define CFGVAR_INITIAL_BUFFERS        "initial-buffers"
#define CFGVAR_SEARCH_HIGHLIGHT       "search-highlight"
#define CFGVAR_SEARCH_HIGHLIGHT_EXACT "search-highlight-exact"
#define CFGVAR_MENU_HIGHLIGHT         "menu-highlight"
#define CFGVAR_DISABLE_QUIT_KEYBIND   "disable-quit-keybind"

extern struct {
        uint32_t   flags;
        char       config_filepath[512];
        size_t     starting_lineno;
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
} glconf;

#endif // GLCONF_H_INCLUDED
