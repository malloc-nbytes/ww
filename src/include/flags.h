#ifndef FLAGS_H_INCLUDED
#define FLAGS_H_INCLUDED

#define CONFIG_FILENAME ".wwrc"
#define DEFAULT_SPACE_AMT 8
#define DEFAULT_COMPILE_COMMAND "make"
#define DEFAULT_TO_CLIPBOARD_COMMAND "echo '%s' | xclip -selection clipboard"

#define FLAG1HELP 'h'
#define FLAG1VERESION 'v'

#define FLAG1CPL {                              \
                FLAG1HELP,                      \
                FLAG1VERESION,                  \
        }

#define FLAG2HELP "help"

#define FLAG2CPL {                              \
                FLAG2HELP,                      \
        }

enum {
        FT_SHOWTRAILS = 1 << 0,
        FT_TABMODE = 1 << 1,
};

void usage(void);
void version(void);

#endif // FLAGS_H_INCLUDED
