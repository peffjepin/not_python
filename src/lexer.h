#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include <stdlib.h>

#include "keywords.h"
#include "operators.h"

typedef enum {
    NULL_TOKEN,
    KEYWORD,
    COMMA,
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
        ShortStr string;
        Keyword keyword;
        Operator operator;
    };
} Token;

typedef struct {
    FILE* srcfile;
    unsigned int current_line;
    unsigned int current_col;
} Lexer;

#define OVERFLOW_CHECK_TOK(tok)                                                          \
    do {                                                                                 \
        if (tok.string.length >= SHORT_STR_CAP - 1) {                                    \
            fprintf(                                                                     \
                stderr, "ERROR: short string overflow at %u:%u\n", tok.line, tok.col     \
            );                                                                           \
            exit(1);                                                                     \
        }                                                                                \
    } while (0)

Token next_token(Lexer* lex);

#endif
