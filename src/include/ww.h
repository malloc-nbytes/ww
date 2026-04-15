#ifndef WW_H_INCLUDED
#define WW_H_INCLUDED

#include "buffer.h"

#include <stddef.h>
#include <stdint.h>

typedef struct ww {
        bufferp_ar  buffers;
        buffer     *monitors[4];
        uint8_t     ab;
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
