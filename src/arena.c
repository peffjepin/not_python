#include "arena.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TODO: some unit tests for this module would be nice

// TODO: tokens probably don't need to be put into the arena memory,
// rather the parser will parse tokens into appropriate structures
// which will then be stored into the arena memory

// TODO: lots of repitition in here could probably be handled better with some macros

static inline void
out_of_memory()
{
    fprintf(stderr, "ERROR: out of memory");
    exit(1);
}

ArenaRef
arena_put_token(Arena* arena, Token token)
{
    if (arena->tokens_count == arena->tokens_capacity) {
        arena->tokens_capacity += ARENA_STRUCT_CHUNK_SIZE;
        arena->tokens = realloc(arena->tokens, sizeof(Token) * (arena->tokens_capacity));
        if (!arena->tokens) out_of_memory();
    }
    arena->tokens[arena->tokens_count] = token;
    return arena->tokens_count++;
}

Token
arena_get_token(Arena* arena, ArenaRef ref)
{
    if (ref >= arena->tokens_count) {
        Token null_token = {.type = NULL_TOKEN};
        return null_token;
    }
    return arena->tokens[ref];
}

ArenaRef
arena_put_statement(Arena* arena, Statement stmt)
{
    if (arena->statements_count == arena->statements_capacity) {
        arena->statements_capacity += ARENA_STRUCT_CHUNK_SIZE;
        arena->statements =
            realloc(arena->statements, sizeof(Statement) * (arena->statements_capacity));
        if (!arena->statements) out_of_memory();
    }
    arena->statements[arena->statements_count] = stmt;
    return arena->statements_count++;
}

Statement
arena_get_statement(Arena* arena, ArenaRef ref)
{
    if (ref >= arena->statements_count) {
        Statement null_stmt = {.kind = NULL_STMT};
        return null_stmt;
    }
    return arena->statements[ref];
}

ArenaRef
arena_put_expression(Arena* arena, Expression expr)
{
    if (arena->expressions_count == arena->expressions_capacity) {
        arena->expressions_capacity += ARENA_STRUCT_CHUNK_SIZE;
        arena->expressions = realloc(
            arena->expressions, sizeof(Expression) * (arena->expressions_capacity)
        );
        if (!arena->expressions) out_of_memory();
    }
    arena->expressions[arena->expressions_count] = expr;
    return arena->expressions_count++;
}

Expression
arena_get_expression(Arena* arena, ArenaRef ref)
{
    if (ref >= arena->expressions_count) {
        fprintf(stderr, "expression ref out of range\n");
        exit(1);
    }
    return arena->expressions[ref];
}

ArenaRef
arena_put_arguments(Arena* arena, Arguments args)
{
    if (arena->arguments_count == arena->arguments_capacity) {
        arena->arguments_capacity += ARENA_STRUCT_CHUNK_SIZE;
        arena->arguments =
            realloc(arena->arguments, sizeof(Arguments) * (arena->arguments_capacity));
        if (!arena->arguments) out_of_memory();
    }
    arena->arguments[arena->arguments_count] = args;
    return arena->arguments_count++;
}

Arguments
arena_get_arguments(Arena* arena, ArenaRef ref)
{
    if (ref >= arena->arguments_count) {
        fprintf(stderr, "arguments ref out of range\n");
        exit(1);
    }
    return arena->arguments[ref];
}

ArenaRef
arena_put_enclosure(Arena* arena, Enclosure enclosure)
{
    if (arena->enclosures_count == arena->enclosures_capacity) {
        arena->enclosures_capacity += ARENA_STRUCT_CHUNK_SIZE;
        arena->enclosures =
            realloc(arena->enclosures, sizeof(Enclosure) * (arena->enclosures_capacity));
        if (!arena->enclosures) out_of_memory();
    }
    arena->enclosures[arena->enclosures_count] = enclosure;
    return arena->enclosures_count++;
}

Enclosure
arena_get_enclosure(Arena* arena, ArenaRef ref)
{
    if (ref >= arena->enclosures_count) {
        fprintf(stderr, "enclosure ref out of range\n");
        exit(1);
    }
    return arena->enclosures[ref];
}

ArenaRef
arena_put_comprehension(Arena* arena, Comprehension comp)
{
    if (arena->comprehensions_count == arena->comprehensions_capacity) {
        arena->comprehensions_capacity += ARENA_STRUCT_CHUNK_SIZE;
        arena->comprehensions = realloc(
            arena->comprehensions,
            sizeof(Comprehension) * (arena->comprehensions_capacity)
        );
        if (!arena->comprehensions) out_of_memory();
    }
    arena->comprehensions[arena->comprehensions_count] = comp;
    return arena->comprehensions_count++;
}

Comprehension
arena_get_comprehension(Arena* arena, ArenaRef ref)
{
    if (ref >= arena->comprehensions_count) {
        fprintf(stderr, "comprehension ref out of range\n");
        exit(1);
    }
    return arena->comprehensions[ref];
}

// TODO: handle duplicate strings
// TODO: often strlen is already known -- implement arena_put_stringl
ArenaRef
arena_put_string(Arena* arena, char* string)
{
    size_t string_length = strlen(string) + 1;  // counting null byte here
    assert(
        string_length <= ARENA_STRING_CHUNK_SIZE &&
        "insanely long string literals not currently supported"
    );
    if (arena->strings_buffer_write_head + string_length >
        arena->strings_buffer_capacity) {
        arena->strings_buffer_capacity += ARENA_STRING_CHUNK_SIZE;
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
arena_get_string(Arena* arena, ArenaRef ref)
{
    if (ref >= arena->strings_buffer_write_head) return NULL;
    return arena->strings_buffer + ref;
}
