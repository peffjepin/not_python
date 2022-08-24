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
    size_t expressions_capacity;
    size_t expressions_count;
    Arguments* arguments;
    size_t arguments_capacity;
    size_t arguments_count;
    Enclosure* enclosures;
    size_t enclosures_capacity;
    size_t enclosures_count;
    Comprehension* comprehensions;
    size_t comprehensions_capacity;
    size_t comprehensions_count;
    Slice* slices;
    size_t slices_capacity;
    size_t slices_count;
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

ArenaRef arena_put_arguments(Arena* arena, Arguments args);
Arguments arena_get_arguments(Arena* arena, ArenaRef ref);

ArenaRef arena_put_enclosure(Arena* arena, Enclosure enclosure);
Enclosure arena_get_enclosure(Arena* arena, ArenaRef ref);

ArenaRef arena_put_comprehension(Arena* arena, Comprehension comp);
Comprehension arena_get_comprehension(Arena* arena, ArenaRef ref);

ArenaRef arena_put_slice(Arena* arena, Slice slice);
Slice arena_get_slice(Arena* arena, ArenaRef ref);

ArenaRef arena_put_string(Arena* arena, char* string);
char* arena_get_string(Arena* arena, ArenaRef ref);

#endif
