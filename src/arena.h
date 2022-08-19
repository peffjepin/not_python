#ifndef ARENA_H
#define ARENA_H

#include "compiler_types.h"
#include "lexer_types.h"

typedef struct {
    Token* tokens;
    size_t tokens_capacity;
    size_t tokens_count;
    Instruction* instructions;
    size_t instructions_capacity;
    size_t instructions_count;
    char* strings_buffer;
    size_t strings_buffer_capacity;
    size_t strings_buffer_write_head;
} Arena;

size_t arena_put_token(Arena* arena, Token token);
Token arena_get_token(Arena* arena, size_t id);

size_t arena_put_instruction(Arena* arena, Instruction inst);
Instruction arena_get_instruction(Arena* arena, size_t id);

size_t arena_put_string(Arena* arena, char* string);
char* arena_get_string(Arena* arena, size_t id);

#endif
