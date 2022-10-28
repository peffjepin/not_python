#include "arena.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "diagnostics.h"

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

static size_t
find_dynamic_chunk(Arena* arena, size_t required_chunks)
{
    bool zero[required_chunks];
    memset(zero, 0, sizeof(zero));

    for (size_t off = 0, stride = 8; off < stride; off++) {
        size_t chunk = off;
        while (chunk < ARENA_DYNAMIC_CHUNK_COUNT) {
            if (memcmp(zero, arena->dynamic_chunks_in_use + chunk, sizeof(zero)) == 0)
                return chunk;
            chunk += stride;
        }
    }

    error("unable to find an available dynamic memory chunk");
    UNREACHABLE();
}

static void
assert_is_dynamic_allocation(Arena* arena, void* allocation)
{
    assert((uint8_t*)allocation >= (uint8_t*)arena->dynamic_region);
    assert(
        (uint8_t*)allocation <=
        (uint8_t*)arena->dynamic_region[ARENA_DYNAMIC_CHUNK_COUNT - 1]
    );
}

static size_t
dynamic_allocation_to_chunk(Arena* arena, void* dynamic_allocation)
{
    // assert `dynamic_alloction` is indeed located in the dynamic allocation region
    uint8_t* head = (uint8_t*)dynamic_allocation - sizeof(size_t);
    size_t offset = head - (uint8_t*)arena->dynamic_region;
    assert(offset % ARENA_DYNAMIC_CHUNK_SIZE == 0);
    return offset / ARENA_DYNAMIC_CHUNK_SIZE;
}

#define REQUIRED_DYNAMIC_CHUNKS(size)                                                    \
    (1 + ((sizeof(size_t) + size) / ARENA_DYNAMIC_CHUNK_SIZE))

void*
arena_dynamic_alloc(Arena* arena, size_t size)
{
    assert(size > 0);

    size_t required_chunks = REQUIRED_DYNAMIC_CHUNKS(size);
    assert(required_chunks < ARENA_DYNAMIC_CHUNK_COUNT);

    size_t chunk = find_dynamic_chunk(arena, required_chunks);

    memset(arena->dynamic_chunks_in_use + chunk, 1, sizeof(bool) * required_chunks);
    uint8_t* head = arena->dynamic_region[chunk];
    *((size_t*)head) = size;
    void* allocation = (void*)(head + sizeof(size_t));
    memset(allocation, 0, size);
    return allocation;
}

void*
arena_dynamic_realloc(Arena* arena, void* allocation, size_t size)
{
    assert_is_dynamic_allocation(arena, allocation);

    size_t required_chunks = REQUIRED_DYNAMIC_CHUNKS(size);
    assert(required_chunks < ARENA_DYNAMIC_CHUNK_COUNT);

    size_t* size_tag = (size_t*)((uint8_t*)allocation - sizeof(size_t));
    size_t prev_required_chunks = REQUIRED_DYNAMIC_CHUNKS(*size_tag);

    // if we don't have a change in number of required chunks just update size
    // and return the same alllocation
    if (prev_required_chunks == required_chunks) {
        *size_tag = size;
        return allocation;
    }

    size_t prev_chunk = dynamic_allocation_to_chunk(arena, allocation);

    // if more chunks are now required first check if the allocation will need to move
    if (prev_required_chunks < required_chunks) {
        bool zero[required_chunks - prev_required_chunks];
        memset(zero, 0, sizeof(zero));
        if (memcmp(
                zero,
                arena->dynamic_chunks_in_use + prev_chunk + prev_required_chunks,
                sizeof(zero)
            ) == 0) {
            // forward space is free, no move required:
            //      mark chunks in use
            //      zero init new region
            //      update size tag
            //      return the same allocation
            memset(
                arena->dynamic_chunks_in_use + prev_chunk,
                1,
                sizeof(bool) * required_chunks
            );
            memset((uint8_t*)allocation + *size_tag, 0, size - *size_tag);
            *size_tag = size;
            return allocation;
        }
        else {
            // forward space is not free, move required:
            //      find a new allocation
            //      copy old data into new buffer and zero initialize the end
            //      mark old chunks not in use
            //      return new allocation
            size_t new_chunk = find_dynamic_chunk(arena, required_chunks);
            memset(
                arena->dynamic_chunks_in_use + new_chunk,
                1,
                sizeof(bool) * required_chunks
            );
            uint8_t* head = arena->dynamic_region[new_chunk];
            *((size_t*)head) = size;
            void* new = (void*)(head + sizeof(size_t));
            memcpy(new, allocation, *size_tag);
            memset((uint8_t*)new + *size_tag, 0, size - *size_tag);
            memset(
                arena->dynamic_chunks_in_use + prev_chunk,
                0,
                sizeof(bool) * prev_required_chunks
            );
            return new;
        }
    }
    // if shrinking update the size and chunks in use then return the same allocation
    else {
        *size_tag = size;
        memset(
            arena->dynamic_chunks_in_use + prev_chunk + required_chunks,
            0,
            sizeof(bool) * (prev_required_chunks - required_chunks)
        );
        return allocation;
    }
    UNREACHABLE();
}

void*
arena_dynamic_finalize(Arena* arena, void* dynamic_allocation)
{
    assert_is_dynamic_allocation(arena, dynamic_allocation);
    uint8_t* head = (uint8_t*)dynamic_allocation - sizeof(size_t);
    void* new = arena_copy(arena, dynamic_allocation, *(size_t*)head);
    arena_dynamic_free(arena, dynamic_allocation);
    return new;
}

void
arena_dynamic_free(Arena* arena, void* dynamic_allocation)
{
    assert_is_dynamic_allocation(arena, dynamic_allocation);
    size_t chunk = dynamic_allocation_to_chunk(arena, dynamic_allocation);
    size_t* size_tag = (size_t*)((uint8_t*)dynamic_allocation - sizeof(size_t));
    size_t nchunks = REQUIRED_DYNAMIC_CHUNKS(*size_tag);
    memset(arena->dynamic_chunks_in_use + chunk, 0, sizeof(bool) * nchunks);
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
    first_static_chunk->buffer = calloc(1, ARENA_STATIC_CHUNK_MIN_SIZE);
    if (!first_static_chunk->buffer) out_of_static_memory();
    first_static_chunk->capacity = ARENA_STATIC_CHUNK_MIN_SIZE;
    return arena;
}

const char*
arena_snprintf(Arena* arena, size_t bufsize, const char* fmt, ...)
{
    void* buf = arena_alloc(arena, bufsize);
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, bufsize, fmt, args);
    va_end(args);
    return buf;
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
