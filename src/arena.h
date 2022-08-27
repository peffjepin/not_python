#ifndef ARENA_H
#define ARENA_H

#include <stdint.h>

#include "lexer_types.h"

typedef struct {
    uint8_t* memory;
    size_t head;
    size_t capacity;
#if DEBUG
    Token* tokens;
    size_t tokens_capacity;
    size_t tokens_count;
#endif
} Arena;

ArenaRef arena_put_memory(Arena* arena, void* data, size_t nbytes);
void* arena_get_memory(Arena* arena, ArenaRef ref);
void arena_free(Arena* arena);

#if DEBUG
void arena_put_token(Arena* arena, Token token);
#endif

#endif
