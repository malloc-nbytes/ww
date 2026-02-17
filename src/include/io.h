#ifndef IO_H_INCLUDED
#define IO_H_INCLUDED

#include "array.h"

int   file_exists(const char *fp);
int   create_file(const char *fp, int force_overwrite);
int   is_dir(const char *path);
int   write_file(const char *fp, const char *content);
char *load_file(const char *path);

cstr_array lsdir(const char *path);
cstr_array walkdir(const char *path);

#endif // IO_H_INCLUDED
