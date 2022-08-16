#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include <stdlib.h>

#include "tokens.h"

typedef struct {
    FILE* srcfile;
    unsigned int current_line;
    unsigned int current_col;
    unsigned int open_parens;
    unsigned int open_curly;
    unsigned int open_square;
} Scanner;

typedef struct {
    char* filepath;
    Scanner scanner;
} Lexer;

Token next_token(Scanner* scanner);
Lexer lex_open(char* filepath);
void lex_close(Lexer* lexer);

#endif
