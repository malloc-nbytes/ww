#ifndef PAIR_H_INCLUDED
#define PAIR_H_INCLUDED

#define PAIR_DEFINE(ty0, ty1, name) \
        typedef struct { \
                ty0 l; \
                ty1 r; \
        } name; \
        \
        name name##_create(ty0 l, ty1 r); \
        ty0 name##_getl(name *p); \
        ty1 name##_getr(name *p)

#define PAIR_IMPL(ty0, ty1, name) \
        name \
        name##_create(ty0 l, ty1 r) \
        { \
                return (name) { .l = l, .r = r, }; \
        } \
        \
        ty0 \
        name##_getl(name *p) \
        { \
                return p->l; \
        } \
        \
        ty1 \
        name##_getr(name *p) \
        { \
                return p->r; \
        }

#endif // PAIR_H_INCLUDED
