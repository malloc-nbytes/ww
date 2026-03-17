#ifndef FLAGS_H_INCLUDED
#define FLAGS_H_INCLUDED

#define CONFIG_FILENAME              ".wwrc"
#define DEFAULT_SPACE_AMT            8
#define DEFAULT_COMPILE_COMMAND      "make"
#define DEFAULT_TO_CLIPBOARD_COMMAND "echo '%s' | xclip -selection clipboard"
#define DEFAULT_EMPTY_LINE_SQUIGGLES 1
#define DEFAULT_SELECTION_HIGHLIGHT  INVERT
#define DEFAULT_INITIAL_BUFFERS      {"ww-help", NULL}

#define FLAG1HELP     'h'
#define FLAG1VERESION 'v'

#define FLAG1CPL { \
        FLAG1HELP, \
        FLAG1VERESION, \
}

#define FLAG2HELP      "help"
#define FLAG2VERSION   "version"
#define FLAG2COPYING   "copying"
#define FLAG2REPLACE   "replace"
#define FLAG2DEFCONFIG "create-default-config"

#define FLAG2CPL { \
                FLAG2HELP, \
                FLAG2VERSION, \
                FLAG2COPYING, \
                FLAG2REPLACE, \
                FLAG2DEFCONFIG, \
        }

enum {
        FT_SHOWTRAILS = 1 << 0,
        FT_TABMODE    = 1 << 1,
        FT_REPLACE    = 1 << 2,
};

#endif // FLAGS_H_INCLUDED
