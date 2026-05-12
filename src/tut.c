/*
 * ww: a simple editor
 * Copyright (C) 2026 malloc-nbytes
 * Contact: zdhdev@yahoo.com

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <https://www.gnu.org/licenses/>.
 */

#include "tut.h"
#include "glconf.h"

#include <assert.h>
#include <string.h>

char *g_tut01 = "*** ww Tutorial Chapter 1 - Basic Navigation ***\n"
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
"  M-< -> jump to top of buffer\n"
"  M-> -> jump to bottom of buffer\n"
"\n"
"Feel free to try these out in this buffer as well.\n"
"\n"
"Invoke `C-x k' to kill this buffer\n"
"\n"
"END OF TUTORIAL\n";

char *g_tut02 = "*** ww Tutorial Chapter 2 - Basic Editing ***\n"
"\n"
"This chapter goes over basic editing of documents.\n"
"\n"
"Typing any of the regular keys on the keyboard will insert them into the buffer.\n"
"You can use BACKSPACE to delete characters behind the cursor, or `C-d' to delete the character under the cursor.\n"
"There are *many* more editing shortcuts that can be used, see `M-x help' to view more.\n"
"\n"
"While in a buffer, there are a few different states that the buffer can be in:\n"
"  - normal    -> can freely type characters\n"
"  - search    -> searching for a pattern\n"
"  - selection -> highlight areas where the cursor is\n"
"  - auto      -> (hidden) prepared to accept a word completion from autocomplete.\n"
"\n"
"When you are finished editing a buffer, use `C-x C-s' to save.\n"
"\n"
"END OF TUTORIAL\n";

char *g_tut03 = "*** ww Tutorial Chapter 3 - Status Bar ***\n"
"\n"
"This chapter explains every entry in the status bar.\n"
"\n"
"Going from left to right, it is:\n"
"  - ww version\n"
"  - filename:line:column (and an asterisk if the buffer has unsaved changes)\n"
"  - buffer state\n"
"  - the current active monitor\n"
"  - space/tab mode and how many spaces/tab width are inserted\n"
"\n"
"END OF TUTORIAL\n";

char *g_tut04 = "*** ww Tutorial Chapter 4 - Autocomplete ***\n"
"\n"
"Upon saving a buffer, it commits any new words it finds into an autocomplete dictionary.\n"
"\n"
"While typing a word, you can at any point hit TAB (your cursor must be *directly in front* of a word)\n"
"and it will start giving suggestions. Continue to hit TAB to see more suggestions.\n"
"To accept a completion, just hit either ENTER or `M-/'.\n"
"\n"
"If the word that is being typed is fairly unique, you can do `M-/' (without hitting TAB first) to\n"
"just quickly accept the first autocomplete entry without displaying the suggestions.\n"
"\n"
"END OF TUTORIAL\n";

buffer *
tut_alloc(ww *ed, const char *chapter)
{
        const char *name;
        linep_ar lns;
        int writable = 0;

        name = chapter;

        if (!strcmp(chapter, TUT_CH1_NAME))
                lns = lines_from(g_tut01);
        else if (!strcmp(chapter, TUT_CH2_NAME))
                lns = lines_from(g_tut02);
        else if (!strcmp(chapter, TUT_CH3_NAME))
                lns = lines_from(g_tut03);
        else if (!strcmp(chapter, TUT_CH4_NAME))
                lns = lines_from(g_tut04);
        else
                return NULL;

        buffer *b = buffer_from(str_from(name),
                               str_from(name),
                               (unsigned)glconf.term.w,
                               (unsigned)glconf.term.h,
                               0, 0, lns, ed);

        if (!writable)
                buffer_make_readonly(b);

        return b;
}
