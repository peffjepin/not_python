#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include <stdlib.h>

#include "arena.h"
#include "compiler_types.h"
#include "lexer_types.h"

typedef struct {
    const char* filepath;
    FILE* srcfile;
    Arena arena;
} Lexer;

Lexer lexer_open(const char* filepath);
void lexer_close(Lexer* lexer);
void lexer_tokenize(Lexer* lexer);
void lexer_parse_instructions(Lexer* lexer);

#endif
