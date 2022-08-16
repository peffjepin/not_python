#ifndef TOKENS_H
#define TOKENS_H

#include "keywords.h"
#include "operators.h"

typedef enum {
    NULL_TOKEN,
    TOK_KEYWORD,
    TOK_COMMA,
    TOK_COLON,
    TOK_STRING,
    TOK_NUMBER,
    TOK_OPERATOR,
    TOK_NEWLINE,
    TOK_BLOCK_BEGIN,
    TOK_BLOCK_END,
    TOK_OPEN_PARENS,
    TOK_CLOSE_PARENS,
    TOK_OPEN_SQUARE,
    TOK_CLOSE_SQUARE,
    TOK_OPEN_CURLY,
    TOK_CLOSE_CURLY,
    TOK_NAME,
    TOK_DOT,
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

#define OVERFLOW_CHECK_TOK(tok)                                                          \
    do {                                                                                 \
        if (tok.string.length >= SHORT_STR_CAP - 1) {                                    \
            fprintf(                                                                     \
                stderr, "ERROR: short string overflow at %u:%u\n", tok.line, tok.col     \
            );                                                                           \
            exit(1);                                                                     \
        }                                                                                \
    } while (0)

#endif
