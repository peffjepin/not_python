#ifndef LEXER_TYPES_H
#define LEXER_TYPES_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    NOT_A_KEYWORD = 0,
    KW_AND = 17,
    KW_AS = 67,
    KW_ASSERT = 78,
    KW_BREAK = 82,
    KW_CLASS = 99,
    KW_CONTINUE = 144,
    KW_DEF = 13,
    KW_DEL = 19,
    KW_ELIF = 126,
    KW_ELSE = 135,
    KW_EXCEPT = 69,
    KW_FINALLY = 26,
    KW_FOR = 37,
    KW_FROM = 1,
    KW_GLOBAL = 45,
    KW_IF = 62,
    KW_IMPORT = 87,
    KW_IN = 70,
    KW_IS = 75,
    KW_LAMBDA = 29,
    KW_NONLOCAL = 129,
    KW_NOT = 47,
    KW_OR = 80,
    KW_PASS = 4,
    KW_RAISE = 97,
    KW_RETURN = 92,
    KW_TRY = 61,
    KW_WHILE = 102,
    KW_WITH = 9,
    KW_YIELD = 100,
} Keyword;

// returns either NOT_A_KEYWORD or KW_*
Keyword is_keyword(char* word);

typedef enum {
    OPERATOR_PLUS = 16,
    OPERATOR_MINUS = 20,
    OPERATOR_MULT = 14,
    OPERATOR_DIV = 24,
    OPERATOR_MOD = 4,
    OPERATOR_POW = 56,
    OPERATOR_FLOORDIV = 1,
    OPERATOR_ASSIGNMENT = 52,
    OPERATOR_PLUS_ASSIGNMENT = 7,
    OPERATOR_MINUS_ASSIGNMENT = 11,
    OPERATOR_MULT_ASSIGNMENT = 5,
    OPERATOR_DIV_ASSIGNMENT = 15,
    OPERATOR_MOD_ASSIGNMENT = 65,
    OPERATOR_FLOORDIV_ASSIGNMENT = 62,
    OPERATOR_POW_ASSIGNMENT = 47,
    OPERATOR_AND_ASSIGNMENT = 67,
    OPERATOR_OR_ASSIGNMENT = 29,
    OPERATOR_XOR_ASSIGNMENT = 39,
    OPERATOR_RSHIFT_ASSIGNMENT = 37,
    OPERATOR_LSHIFT_ASSIGNMENT = 31,
    OPERATOR_EQUAL = 43,
    OPERATOR_NOT_EQUAL = 57,
    OPERATOR_GREATER = 54,
    OPERATOR_LESS = 50,
    OPERATOR_GREATER_EQUAL = 45,
    OPERATOR_LESS_EQUAL = 41,
    OPERATOR_BITWISE_AND = 6,
    OPERATOR_BITWISE_OR = 38,
    OPERATOR_BITWISE_XOR = 48,
    OPERATOR_BITWISE_NOT = 42,
    OPERATOR_LSHIFT = 40,
    OPERATOR_RSHIFT = 46,
} Operator;

#define MAX_PRECEDENCE 14
extern const unsigned int PRECENDENCE_TABLE[70];

extern const bool IS_ASSIGNMENT_OP[70];
Operator op_from_cstr(char* op);

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
    TOK_IDENTIFIER,
    TOK_DOT,
    TOK_EOF,
} TokenType;

typedef struct {
    const char* filename;
    unsigned int col;
    unsigned int line;
} Location;

typedef struct {
    Location loc;
    TokenType type;
    size_t id;
    size_t value_ref;
} Token;

#endif
