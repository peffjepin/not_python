#ifndef ARENA_H
#define ARENA_H

#include <stdint.h>

#include "lexer_types.h"

typedef struct {
    uint8_t* buffer;
    size_t capacity;
    size_t head;
} ArenaStaticChunk;

#define ARENA_DYNAMIC_CHUNK_SIZE 1024
#define ARENA_DYNAMIC_CHUNK_COUNT 1024

typedef struct {
    size_t static_chunks_count;
    size_t static_chunks_capacity;
    size_t current_static_chunk;
    ArenaStaticChunk* static_chunks;
    uint8_t dynamic_chunks[ARENA_DYNAMIC_CHUNK_COUNT][ARENA_DYNAMIC_CHUNK_SIZE];
    bool dynamic_chunks_in_use[ARENA_DYNAMIC_CHUNK_COUNT];
// TODO: expose some tokenization function from lexer.c instead of doing this
#if DEBUG
    Token* tokens;
    size_t tokens_capacity;
    size_t tokens_count;
#endif
} Arena;

Arena* arena_init(void);
void arena_free(Arena* arena);

// returns a pointer to a dynamic chunk of arena memory
void* arena_dynamic_alloc(Arena* arena, size_t* capacity);

// when a dynamic arena allocation needs to grow this function requests another chunk
void* arena_dynamic_grow(Arena* arena, void* dynamic_allocation, size_t* capacity);

// once a dynamic allocation is no longer growing it can be shifted into static arena
// memory with this function
void* arena_dynamic_finalize(Arena* arena, void* dynamic_allocation, size_t nbytes);

// for static allocations
void* arena_alloc(Arena* arena, size_t nbytes);
// copy into static arena memory
void* arena_copy(Arena* arena, void* data, size_t nbytes);

#if DEBUG
void arena_put_token(Arena* arena, Token token);
#endif

#endif
