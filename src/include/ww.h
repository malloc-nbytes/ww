#ifndef WW_H_INCLUDED
#define WW_H_INCLUDED

#include "buffer.h"

#include <stddef.h>
#include <stdint.h>

#define WW_CMD_SAVE             "save-file"
#define WW_CMD_FIND_FILE        "find-file"
#define WW_CMD_COMPILE          "compile"
#define WW_CMD_TOGGLE_SPACEMODE "toggle-spacemode"
#define WW_CMD_SPACEAMT         "space-amt"

#define WW_CMD_CPL { \
        WW_CMD_SAVE, \
        WW_CMD_FIND_FILE, \
        WW_CMD_COMPILE, \
        WW_CMD_TOGGLE_SPACEMODE, \
        WW_CMD_SPACEAMT, \
}

typedef struct ww {
        bufferp_ar  buffers;
        buffer     *monitors[4];
        uint8_t     am;
} ww;

ww   ww_create(void);
void ww_run(ww *ed);
int  ww_buffer_exists_by_name(const ww *ed, const char *name);
int  ww_buffer_exists_by_path(const ww *ed, const char *path);
void ww_add_buffer(ww *ed, buffer *b);
void ww_make_buffer_primary(ww *ed, size_t idx);
void ww_make_buffer_primary_by_path(ww *ed, const char *path);
void ww_clear_monitors(ww *ed);
void ww_display_monitors(ww *ed, buffer_action ba);
void ww_switch_buffer(ww *ed);
void ww_make_buffer_primary_by_name(ww *ed, const char *name);

#endif // WW_H_INCLUDED
