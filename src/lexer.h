#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include <stdlib.h>

#include "arena.h"
#include "syntax.h"
#include "lexer_types.h"

typedef struct {
    Arena* arena;
    size_t n_statements;
    Statement* statements;
} Lexer;

Lexer lex_file(const char* filepath);
void lexer_free(Lexer* lexer);

#endif
