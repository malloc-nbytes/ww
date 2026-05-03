#ifndef GLCONF_H_INCLUDED
#define GLCONF_H_INCLUDED

#include <stddef.h>
#include <termios.h>

extern struct {
        struct {
                size_t         w;
                size_t         h;
                struct termios termios;
        } term;
        struct {
                char *compile;
                int   space_amt;
        } runtime;
        uint32_t flags;
} glconf;

#endif // GLCONF_H_INCLUDED
