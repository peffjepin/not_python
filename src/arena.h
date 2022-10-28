#ifndef ARENA_H
#define ARENA_H

#include <stdint.h>

#include "generated.h"

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
    uint8_t dynamic_region[ARENA_DYNAMIC_CHUNK_COUNT][ARENA_DYNAMIC_CHUNK_SIZE];
    bool dynamic_chunks_in_use[ARENA_DYNAMIC_CHUNK_COUNT];
} Arena;

Arena* arena_init(void);
void arena_free(Arena* arena);

// returns a pointer to a dynamic chunk of arena memory
void* arena_dynamic_alloc(Arena* arena, size_t size);

// when a dynamic arena allocation needs to grow this function requests another chunk
void* arena_dynamic_realloc(Arena* arena, void* dynamic_allocation, size_t size);

// once a dynamic allocation is no longer growing it can be shifted into static arena
// memory with this function
void* arena_dynamic_finalize(Arena* arena, void* dynamic_allocation);

// for when arena_dynamic_finalize is not applicable
void arena_dynamic_free(Arena* arena, void* dynamic_allocation);

// for static allocations
void* arena_alloc(Arena* arena, size_t nbytes);

// copy into static arena memory
void* arena_copy(Arena* arena, void* data, size_t nbytes);

const char* arena_snprintf(Arena* arena, size_t bufsize, const char* fmt, ...);

#endif
