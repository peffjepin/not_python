#ifndef LEXER_TYPES_H
#define LEXER_TYPES_H

#include <stdbool.h>
#include <stddef.h>

#include "forward.h"

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

const char* kw_to_cstr(Keyword kw);

#define OPERATOR_TABLE_MAX 135

typedef enum {
    OPERATOR_PLUS = 86,
    OPERATOR_MINUS = 90,
    OPERATOR_MULT = 84,
    OPERATOR_DIV = 94,
    OPERATOR_MOD = 74,
    OPERATOR_POW = 126,
    OPERATOR_FLOORDIV = 6,
    OPERATOR_ASSIGNMENT = 122,
    OPERATOR_PLUS_ASSIGNMENT = 12,
    OPERATOR_MINUS_ASSIGNMENT = 16,
    OPERATOR_MULT_ASSIGNMENT = 10,
    OPERATOR_DIV_ASSIGNMENT = 20,
    OPERATOR_MOD_ASSIGNMENT = 0,
    OPERATOR_FLOORDIV_ASSIGNMENT = 67,
    OPERATOR_POW_ASSIGNMENT = 52,
    OPERATOR_AND_ASSIGNMENT = 2,
    OPERATOR_OR_ASSIGNMENT = 39,
    OPERATOR_XOR_ASSIGNMENT = 114,
    OPERATOR_RSHIFT_ASSIGNMENT = 112,
    OPERATOR_LSHIFT_ASSIGNMENT = 106,
    OPERATOR_EQUAL = 48,
    OPERATOR_NOT_EQUAL = 127,
    OPERATOR_GREATER = 124,
    OPERATOR_LESS = 120,
    OPERATOR_GREATER_EQUAL = 50,
    OPERATOR_LESS_EQUAL = 46,
    OPERATOR_BITWISE_AND = 76,
    OPERATOR_BITWISE_OR = 113,
    OPERATOR_BITWISE_XOR = 53,
    OPERATOR_BITWISE_NOT = 117,
    OPERATOR_CONDITIONAL_IF = 42,
    OPERATOR_CONDITIONAL_ELSE = 121,
    OPERATOR_LSHIFT = 45,
    OPERATOR_RSHIFT = 51,
    OPERATOR_CALL = 77,
    OPERATOR_GET_ITEM = 11,
    OPERATOR_GET_ATTR = 23,
    OPERATOR_LOGICAL_AND = 134,
    OPERATOR_LOGICAL_OR = 66,
} Operator;

#define MAX_PRECEDENCE 16
extern const unsigned int PRECENDENCE_TABLE[OPERATOR_TABLE_MAX];
extern const bool IS_ASSIGNMENT_OP[OPERATOR_TABLE_MAX];

Operator op_from_cstr(char* op);
const char* op_to_cstr(Operator op);

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

const char* token_type_to_cstr(TokenType type);

typedef struct {
    const char* filename;
    unsigned int col;
    unsigned int line;
} Location;

typedef struct {
    Location loc;
    TokenType type;
    union {
        int value;
        ArenaRef ref;
    };
} Token;

#endif
