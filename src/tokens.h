#ifndef TOKENS_H
#define TOKENS_H

#include <string.h>

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
    char* data;
    size_t length;
} SourceString;

#define SOURCESTRING_EQ(str1, str2)                                                      \
    str1.length == str2.length&& strcmp(str1.data, str2.data) == 0

typedef struct {
    const char* filepath;
    unsigned int col;
    unsigned int line;
} Location;

typedef struct {
    Location* loc;
    TokenType type;
    union {
        Keyword kw;
        Operator op;
        SourceString value;
    };
} Token;

#endif
