#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include <stdlib.h>

#include "arena.h"
#include "generated.h"
#include "syntax.h"

typedef struct {
    Arena* arena;
    size_t n_statements;
    Statement** statements;
    LexicalScope* top_level;
} Lexer;

Lexer lex_file(const char* filepath);
void lexer_free(Lexer* lexer);

#if DEBUG
Token* tokenize_file(const char* filepath, size_t* token_count);
#endif

#endif
