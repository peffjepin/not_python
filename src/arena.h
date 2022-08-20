#ifndef ARENA_H
#define ARENA_H

#include "aliases.h"
#include "compiler_types.h"
#include "lexer_types.h"

#define ARENA_STRUCT_CHUNK_SIZE 256
#define ARENA_STRING_CHUNK_SIZE 4096

typedef struct {
    Token* tokens;
    size_t tokens_capacity;
    size_t tokens_count;
    Statement* statements;
    size_t statements_capacity;
    size_t statements_count;
    Expression* expressions;
    size_t expressions_count;
    size_t expressions_capacity;
    char* strings_buffer;
    size_t strings_buffer_capacity;
    size_t strings_buffer_write_head;
} Arena;

ArenaRef arena_put_token(Arena* arena, Token token);
Token arena_get_token(Arena* arena, ArenaRef ref);

ArenaRef arena_put_statement(Arena* arena, Statement stmt);
Statement arena_get_statement(Arena* arena, ArenaRef ref);

ArenaRef arena_put_expression(Arena* arena, Expression expr);
Expression arena_get_expression(Arena* arena, ArenaRef ref);

ArenaRef arena_put_string(Arena* arena, char* string);
char* arena_get_string(Arena* arena, ArenaRef ref);

#endif
