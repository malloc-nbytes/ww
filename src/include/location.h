#ifndef LOCATION_H_INCLUDED
#define LOCATION_H_INCLUDED

typedef struct {
        char *path;
        unsigned r;
        unsigned c;
} location;

location location_from(char *path, unsigned r, unsigned c);
void location_destroy(location *loc);

#endif // LOCATION_H_INCLUDED
