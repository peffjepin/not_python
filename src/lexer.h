#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include <stdlib.h>

#include "instructions.h"
#include "tokens.h"

typedef struct {
    FILE* srcfile;
    TokenStream ts;
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

Lexer lex_open(char* filepath);
void lex_close(Lexer* lexer);

Token next_token(Scanner* scanner);
void scan_to_token_stream(Scanner* scanner);

Instruction next_instruction(Lexer* lex);

#endif
