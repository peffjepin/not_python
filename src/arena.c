#include "arena.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline void
out_of_static_memory()
{
    fprintf(stderr, "ERROR: out of memory");
    exit(1);
}

static inline void
out_of_dynamic_memory()
{
    fprintf(stderr, "ERROR: arena out of dynamic memory chunks");
    exit(1);
}

void*
arena_dynamic_alloc(Arena* arena, size_t* capacity)
{
    for (size_t i = 0; i < ARENA_DYNAMIC_CHUNK_COUNT; i++) {
        if (!arena->dynamic_chunks_in_use[i]) {
            arena->dynamic_chunks_in_use[i] = true;
            *capacity = ARENA_DYNAMIC_CHUNK_SIZE;
            return arena->dynamic_chunks[i];
        }
    }
    return NULL;
}

static size_t
dynamic_allocation_to_chunk(Arena* arena, void* dynamic_allocation)
{
    // assert `dynamic_alloction` is indeed located in the dynamic allocation region
    assert((uint8_t*)dynamic_allocation >= (uint8_t*)arena->dynamic_chunks);
    assert(
        (uint8_t*)dynamic_allocation <=
        (uint8_t*)arena->dynamic_chunks[ARENA_DYNAMIC_CHUNK_COUNT - 1]
    );
    size_t offset = (uint8_t*)dynamic_allocation - (uint8_t*)arena->dynamic_chunks;
    assert(offset % ARENA_DYNAMIC_CHUNK_SIZE == 0);

    return offset / ARENA_DYNAMIC_CHUNK_SIZE;
}

void*
arena_dynamic_grow(Arena* arena, void* dynamic_allocation, size_t* capacity)
{
    size_t chunk = dynamic_allocation_to_chunk(arena, dynamic_allocation);
    size_t nchunks = *capacity / ARENA_DYNAMIC_CHUNK_SIZE;
    size_t next_chunk = chunk + nchunks;
    if (next_chunk >= ARENA_DYNAMIC_CHUNK_COUNT) goto memory_error;

    // case where the next chunk is empty and we can just reserve it and return
    if (!arena->dynamic_chunks_in_use[chunk + nchunks]) {
        *capacity += ARENA_DYNAMIC_CHUNK_SIZE;
        arena->dynamic_chunks_in_use[chunk + nchunks] = true;
        return dynamic_allocation;
    }

    // case where the next chunk is taken and we have to relocate
    // first we will search for a new region and copy existing memory there
    int new_head_chunk = -1;
    for (size_t i = 0; i < ARENA_DYNAMIC_CHUNK_COUNT; i++) {
        if (arena->dynamic_chunks_in_use[i] == true)
            new_head_chunk = -1;
        else if (new_head_chunk == -1)
            new_head_chunk = i;
        else if (i - new_head_chunk == nchunks)
            break;
    }
    if (new_head_chunk == -1 || new_head_chunk + nchunks >= ARENA_DYNAMIC_CHUNK_COUNT)
        goto memory_error;
    memcpy(
        arena->dynamic_chunks[new_head_chunk],
        dynamic_allocation,
        nchunks * ARENA_DYNAMIC_CHUNK_SIZE
    );
    // then we can clear the in use flags for the chunks we have vacated
    memset(arena->dynamic_chunks_in_use + chunk, false, sizeof(bool) * nchunks);
    // and set the in use flags for the new region
    memset(
        arena->dynamic_chunks_in_use + new_head_chunk, true, sizeof(bool) * (nchunks + 1)
    );

    *capacity += ARENA_DYNAMIC_CHUNK_SIZE;
    return arena->dynamic_chunks[new_head_chunk];

memory_error:
    // At the time of writing this allocator is meant to have more chunks
    // available for dynamic allocation than it would ever need. If this
    // state is reached a redesign is probably due.
    out_of_dynamic_memory();
    return NULL;
}

void*
arena_dynamic_finalize(Arena* arena, void* dynamic_allocation, size_t nbytes)
{
    void* ptr = NULL;
    if (nbytes > 0) {
        ptr = arena_alloc(arena, nbytes);
        if (!ptr) out_of_static_memory();
        memcpy(ptr, dynamic_allocation, nbytes);
    }
    arena_dynamic_free(arena, dynamic_allocation, nbytes);
    return ptr;
}

void
arena_dynamic_free(Arena* arena, void* dynamic_allocation, size_t nbytes)
{
    // TODO: if dynamic_chunks_in_use kept the nchunks value in the head chunk position
    // we could avoid some computation here and not have to pass in nbytes to free chunks
    size_t head_chunk = dynamic_allocation_to_chunk(arena, dynamic_allocation);
    size_t nchunks;
    if (nbytes % ARENA_DYNAMIC_CHUNK_SIZE == 0) {
        nchunks = nbytes / ARENA_DYNAMIC_CHUNK_SIZE;
    }
    else {
        nchunks = nbytes / ARENA_DYNAMIC_CHUNK_SIZE + 1;
    }
    memset(arena->dynamic_chunks_in_use + head_chunk, false, sizeof(bool) * nchunks);
}

#define ARENA_STATIC_CHUNK_MIN_SIZE 4096

static ArenaStaticChunk*
get_next_clean_static_chunk(Arena* arena)
{
    if (arena->static_chunks_count == arena->static_chunks_capacity) {
        arena->static_chunks_capacity += 8;
        arena->static_chunks = realloc(
            arena->static_chunks, sizeof(ArenaStaticChunk) * arena->static_chunks_capacity
        );
        if (!arena->static_chunks) out_of_static_memory();
    }
    return arena->static_chunks + arena->static_chunks_count++;
}

void*
arena_alloc(Arena* arena, size_t nbytes)
{
    if (nbytes == 0) return NULL;

    // large allocations are put into their own fresh chunk
    if (nbytes >= ARENA_STATIC_CHUNK_MIN_SIZE) {
        ArenaStaticChunk* static_chunk = get_next_clean_static_chunk(arena);
        static_chunk->capacity = nbytes;
        static_chunk->head = nbytes;
        static_chunk->buffer = malloc(nbytes);
        if (!static_chunk->buffer) out_of_static_memory();
        memset(static_chunk->buffer, 0, nbytes);
        return static_chunk->buffer;
    }

    // for smaller sizes we try to fit the allocation into the current chunk
    ArenaStaticChunk* existing = arena->static_chunks + arena->current_static_chunk;
    if (existing->head + nbytes <= existing->capacity) {
        void* ptr = existing->buffer + existing->head;
        existing->head += nbytes;
        return ptr;
    }

    // if the current chunk wont fit the allocation we start a new current chunk
    ArenaStaticChunk* new_chunk = get_next_clean_static_chunk(arena);
    arena->current_static_chunk = arena->static_chunks_count - 1;
    new_chunk->buffer = malloc(ARENA_STATIC_CHUNK_MIN_SIZE);
    if (!new_chunk->buffer) out_of_static_memory();
    memset(new_chunk->buffer, 0, ARENA_STATIC_CHUNK_MIN_SIZE);
    new_chunk->capacity = ARENA_STATIC_CHUNK_MIN_SIZE;
    new_chunk->head = nbytes;
    return new_chunk->buffer;
}

void*
arena_copy(Arena* arena, void* data, size_t nbytes)
{
    void* new_ptr = arena_alloc(arena, nbytes);
    memcpy(new_ptr, data, nbytes);
    return new_ptr;
}

Arena*
arena_init(void)
{
    Arena* arena = calloc(1, sizeof(Arena));
    if (!arena) out_of_static_memory();
    // set up first static block
    ArenaStaticChunk* first_static_chunk = get_next_clean_static_chunk(arena);
    first_static_chunk->buffer = malloc(ARENA_STATIC_CHUNK_MIN_SIZE);
    if (!first_static_chunk->buffer) out_of_static_memory();
    first_static_chunk->capacity = ARENA_STATIC_CHUNK_MIN_SIZE;
    return arena;
}

void
arena_free(Arena* arena)
{
    for (size_t i = 0; i < arena->static_chunks_count; i++) {
        free(arena->static_chunks[i].buffer);
    }
    free(arena->static_chunks);
    free(arena);
}
