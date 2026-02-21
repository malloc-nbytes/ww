#include "flags.h"

#include <stdint.h>
#include <stddef.h>
#include <termios.h>

struct {
        uint32_t flags;
        char     config_filepath[512];

        struct {
                int         space_amt;
                const char *compile_cmd;
                const char *to_clipboard;
        } defaults;

        struct {
                size_t w;
                size_t h;
                struct termios old;
        } term;
} glconf = {
        .flags           = 0x0000,
        .config_filepath = {0},
        .defaults = {
                .space_amt    = DEFAULT_SPACE_AMT,
                .compile_cmd  = DEFAULT_COMPILE_COMMAND,
                .to_clipboard = DEFAULT_TO_CLIPBOARD_COMMAND,
        },
        .term = {
                .w   = 0,
                .h   = 0,
                .old = {0},
        },
};
