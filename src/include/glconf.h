#ifndef GLCONF_H_INCLUDED
#define GLCONF_H_INCLUDED

#include "qcl.h"

#include <stddef.h>
#include <stdint.h>
#include <termios.h>

extern struct {
        uint32_t flags;
        char     config_filepath[512];
        size_t   starting_lineno;

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
