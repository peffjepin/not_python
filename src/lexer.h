#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include <stdlib.h>

#include "keywords.h"
#include "operators.h"

typedef enum {
    NULL_TOKEN,
    KEYWORD,
    COLON,
    STRING,
    NUMBER,
    OPERATOR,
    NEWLINE,
    BLOCK_BEGIN,
    BLOCK_END,
    OPEN_PARENS,
    CLOSE_PARENS,
    OPEN_SQUARE,
    CLOSE_SQUARE,
    OPEN_CURLY,
    CLOSE_CURLY,
    NAME,
    DOT,
} TokenType;

#define SHORT_STR_CAP 128

typedef struct {
    char buffer[SHORT_STR_CAP];
    size_t length;
} ShortStr;

typedef struct {
    int line;
    int col;
    TokenType type;
    union {
        ShortStr value;
        Keyword keyword;
        Operator operator;
    };
} Token;

typedef struct {
    FILE* srcfile;
    Token prev;
    unsigned int current_line;
    unsigned int current_col;
} Lexer;

#define OVERFLOW_CHECK_SHORT_STR(str, lex_ptr)                                           \
    do {                                                                                 \
        if (str.length >= SHORT_STR_CAP - 1) {                                           \
            fprintf(                                                                     \
                stderr,                                                                  \
                "ERROR: short string overflow at %u:%u\n",                               \
                lex_ptr->current_line + 1,                                               \
                lex_ptr->current_col + 1                                                 \
            );                                                                           \
            exit(1);                                                                     \
        }                                                                                \
    } while (0)

Token next_token(Lexer* lex);

#endif
