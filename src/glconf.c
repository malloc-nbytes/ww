#include <stddef.h>
#include <stdint.h>
#include <termios.h>

struct {
        struct {
                size_t         w;
                size_t         h;
                struct termios termios;
        } term;
        struct {
                const char *compile;
                int         space_amt;
        } runtime;
        uint32_t flags;
} glconf = {
        .term = {
                .w       = 0,
                .h       = 0,
                .termios = {0},
        },
        .runtime = {
                .compile   = NULL,
                .space_amt = 8,
        },
        .flags = 0x0000,
};
