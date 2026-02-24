#ifndef FLAGS_H_INCLUDED
#define FLAGS_H_INCLUDED

#define CONFIG_FILENAME ".wwrc"
#define DEFAULT_SPACE_AMT 8
#define DEFAULT_COMPILE_COMMAND "make"
#define DEFAULT_TO_CLIPBOARD_COMMAND "echo '%s' | xclip -selection clipboard"

enum {
        FT_SHOWTRAILS = 1 << 0,
        FT_TABMODE = 1 << 1,
};

void usage(void);

#endif // FLAGS_H_INCLUDED
