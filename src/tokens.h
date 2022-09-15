#ifndef TOKENS_H
#define TOKENS_H

#include "generated.h"

// TODO: `@` decorator token

typedef enum {
    NULL_TOKEN,
    TOK_KEYWORD,
    TOK_COMMA,
    TOK_COLON,
    TOK_STRING,
    TOK_NUMBER,
    TOK_OPERATOR,
    TOK_NEWLINE,
    TOK_OPEN_PARENS,
    TOK_CLOSE_PARENS,
    TOK_OPEN_SQUARE,
    TOK_CLOSE_SQUARE,
    TOK_OPEN_CURLY,
    TOK_CLOSE_CURLY,
    TOK_IDENTIFIER,
    TOK_DOT,
    TOK_ARROW,
    TOK_EOF,
} TokenType;

typedef struct {
    const char* filepath;
    unsigned int col;
    unsigned int line;
} Location;

typedef struct {
    Location loc;
    TokenType type;
    union {
        Keyword kw;
        Operator op;
        char* value;
    };
} Token;

#endif
