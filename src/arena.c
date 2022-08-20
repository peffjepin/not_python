#include "arena.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TODO: some unit tests for this module would be nice

// TODO: tokens probably don't need to be put into the arena memory,
// rather the parser will parse tokens into appropriate structures
// which will then be stored into the arena memory

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
        fprintf(stderr, "expression out of range\n");
        exit(1);
    }
    return arena->expressions[ref];
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
