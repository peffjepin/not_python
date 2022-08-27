#ifndef FORWARD_H
#define FORWARD_H

#include <stddef.h>

typedef size_t ArenaRef;

typedef struct {
    ArenaRef ref;
    size_t length;
} ArenaArray;

#endif
