#include "arena.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline void
out_of_memory()
{
    fprintf(stderr, "ERROR: out of memory");
    exit(1);
}

#define ARENA_CHUNK_SIZE 4096

ArenaRef
arena_put_memory(Arena* arena, void* data, size_t nbytes)
{
    if (arena->capacity < arena->head + nbytes) {
        while (arena->capacity < arena->head + nbytes)
            arena->capacity += ARENA_CHUNK_SIZE;
        arena->memory = realloc(arena->memory, arena->capacity);
        if (!arena->memory) out_of_memory();
    }
    memcpy(arena->memory + arena->head, data, nbytes);
    ArenaRef ref = arena->head;
    arena->head += nbytes;
    return ref;
}

void*
arena_get_memory(Arena* arena, ArenaRef ref)
{
    assert(ref < arena->head && "arena ref out of range");
    return arena->memory + ref;
}

void
arena_free(Arena* arena)
{
    free(arena->memory);
#if DEBUG
    free(arena->tokens);
#endif
}

#if DEBUG
void
arena_put_token(Arena* arena, Token token)
{
    if (arena->tokens_count == arena->tokens_capacity) {
        arena->tokens_capacity += 128;
        arena->tokens = realloc(arena->tokens, sizeof(Token) * (arena->tokens_capacity));
        if (!arena->tokens) out_of_memory();
    }
    arena->tokens[arena->tokens_count++] = token;
}
#endif
