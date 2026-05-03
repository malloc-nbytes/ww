#include "tut.h"
#include "glconf.h"

#include <string.h>

char *g_tut01 = "*** ww Tutorial Chapter 1 ***\n"
"\n"
"Welcome to the first tutorial on how to use this editor.\n"
"\n"
"The point of this series of chapters is to teach you how to modify and navigate files.\n"
"It will not go over every single feature and keybinding, but it will go over the essentials.\n"
"\n"
"Before we get started, it is important to understand some notation:\n"
"\n"
"  C -> Control\n"
"  M -> Meta (alt key)\n"
"  - -> Modifier binding\n"
"\n"
"Here are some examples:\n"
"\n"
"  C-x     -> Hold control and hit 'x'\n"
"  M-g     -> Hold meta (alt) and hit 'g'\n"
"  C-x m   -> Hold control and hit 'x', let go of control and hit 'm'\n"
"  C-x C-s -> Hold control and hit 'x', keep holding control and hit 's'\n"
"  C-x -   -> Hold control and hit 'x', let go of control and hit '-'\n"
"\n"
"To start, lets to over some of the basic movement:\n"
"\n"
"  C-n -> move down\n"
"  C-p -> move up\n"
"  C-f -> move forward\n"
"  C-b -> move backward\n"
"\n"
"Try moving the cursor to the box:\n"
"                    [ ]\n"
"Once you are in the box, you can center your view with `C-l'\n"
"\n" "\n" "\n" "\n" "\n" "\n" "\n" "\n"
"This line should now be visible.\n"
"\n"
"Here are some more movement options:\n"
"\n"
"  M-f -> jump forward a word\n"
"  M-b -> jump backward a word\n"
"  C-a -> go to the beginning of line\n"
"  C-e -> go to the end of line\n"
"  M-m -> go to first non-space character on the line\n"
"\n"
"Try it with the following line:\n"
"\n"
"  The quick brown fox jumps over the lazy dog.\n"
"\n"
"This editor also supports text going off the screen.\n"
"Try navigating to the end of this line:\n"
"\n"
"  The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog The quick brown fox jumps over the lazy dog <<< you found the end :) >>>\n"
"\n"
"Here are some more movement options:\n"
"  M-{ -> jump a paragraph backwards\n"
"  M-} -> jump a paragraph forwards\n"
"  M-v -> page up\n"
"  C-v -> page down\n"
"\n"
"Feel free to try these out in this buffer as well.";

buffer *
tut_alloc(ww *ed, const char *chapter)
{
        const char *name;
        linep_ar lns;
        int writable = 0;

        if (!strcmp(chapter, "ch1")) {
                name = "ch1";
                lns = lines_from(g_tut01);
                writable = 0;
        }

        buffer *b = buffer_from(str_from(name),
                               str_from(name),
                               (unsigned)glconf.term.w,
                               (unsigned)glconf.term.h,
                               0, 0, lns, ed);

        if (!writable)
                buffer_make_readonly(b);

        return b;
}
