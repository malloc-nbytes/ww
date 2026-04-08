#include <stddef.h>
#include <termios.h>

struct {
        struct {
                size_t w;
                size_t h;
                struct termios termios;
        } term;
} glconf = {
        .term = {
                .w = 0,
                .h = 0,
                .termios = {0},
        },
};
