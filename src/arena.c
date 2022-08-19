#include "arena.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TODO: some unit tests for this module would be nice

#define CHUNK_SIZE 256
#define STRING_CHUNK_SIZE 4096

static inline void
out_of_memory()
{
    fprintf(stderr, "ERROR: out of memory");
    exit(1);
}

size_t
arena_put_token(Arena* arena, Token token)
{
    if (arena->tokens_capacity == 0) {
        arena->tokens_capacity += CHUNK_SIZE;
        arena->tokens = malloc(sizeof(Token) * arena->tokens_capacity);
        if (!arena->tokens) out_of_memory();
    }
    else if (arena->tokens_count == arena->tokens_capacity) {
        arena->tokens_capacity += CHUNK_SIZE;
        arena->tokens = realloc(arena->tokens, sizeof(Token) * (arena->tokens_capacity));
        if (!arena->tokens) out_of_memory();
    }
    arena->tokens[arena->tokens_count] = token;
    return arena->tokens_count++;
}

Token
arena_get_token(Arena* arena, size_t id)
{
    if (id >= arena->tokens_count) {
        Token null_token = {.type = NULL_TOKEN};
        return null_token;
    }
    return arena->tokens[id];
}

size_t
arena_put_instruction(Arena* arena, Instruction inst)
{
    if (arena->instructions_capacity == 0) {
        arena->instructions_capacity += CHUNK_SIZE;
        arena->instructions = malloc(sizeof(Instruction) * arena->instructions_capacity);
        if (!arena->instructions) out_of_memory();
    }
    else if (arena->instructions_count == arena->instructions_capacity) {
        arena->instructions_capacity += CHUNK_SIZE;
        arena->instructions = realloc(
            arena->instructions, sizeof(Instruction) * (arena->instructions_capacity)
        );
        if (!arena->instructions) out_of_memory();
    }
    arena->instructions[arena->instructions_count] = inst;
    return arena->instructions_count++;
}

Instruction
arena_get_instruction(Arena* arena, size_t id)
{
    if (id >= arena->instructions_count) {
        Instruction null_inst = {.type = NULL_INST};
        return null_inst;
    }
    return arena->instructions[id];
}

size_t
arena_put_string(Arena* arena, char* string)
{
    if (arena->strings_buffer_capacity == 0) {
        arena->strings_buffer_capacity += STRING_CHUNK_SIZE;
        arena->strings_buffer = malloc(arena->strings_buffer_capacity);
        if (!arena->strings_buffer) out_of_memory();
    }
    size_t string_length = strlen(string) + 1;  // counting null byte here
    assert(
        string_length <= STRING_CHUNK_SIZE &&
        "insanely long string literals not currently supported"
    );
    if (arena->strings_buffer_write_head + string_length >
        arena->strings_buffer_capacity) {
        arena->strings_buffer_capacity += STRING_CHUNK_SIZE;
        arena->strings_buffer =
            realloc(arena->strings_buffer, arena->strings_buffer_capacity);
        if (!arena->strings_buffer) out_of_memory();
    }
    memcpy(
        arena->strings_buffer + arena->strings_buffer_write_head, string, string_length
    );
    size_t string_id = arena->strings_buffer_write_head;
    arena->strings_buffer_write_head += string_length;
    return string_id;
}

char*
arena_get_string(Arena* arena, size_t id)
{
    if (id >= arena->strings_buffer_write_head) return NULL;
    return arena->strings_buffer + id;
}
