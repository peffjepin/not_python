#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include <stdlib.h>

#include "instructions.h"
#include "lexer_types.h"

typedef struct {
    const char* filepath;
    FILE* srcfile;
    TokenStream ts;
} Lexer;

Lexer lexer_open(const char* filepath);
void lexer_close(Lexer* lexer);
void lexer_tokenize(Lexer* lexer);

Instruction next_instruction(Lexer* lexer);

#endif
