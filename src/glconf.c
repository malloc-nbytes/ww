#include <stddef.h>
#include <termios.h>

struct {
        struct {
                size_t         w;
                size_t         h;
                struct termios termios;
        } term;
        struct {
                const char *compile;
                int         spacemode;
                int         space_amt;
        } runtime;
} glconf = {
        .term = {
                .w       = 0,
                .h       = 0,
                .termios = {0},
        },
        .runtime = {
                .compile   = NULL,
                .spacemode = 1,
                .space_amt = 8,
        },
};
