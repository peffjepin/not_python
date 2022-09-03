/*
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
this code is generated by `scripts/codegen.py` and should not be edited manually
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*/
#ifndef GENERATED_H
#define GENERATED_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    NOT_A_KEYWORD = 0,
    KW_FALSE = 71,
    KW_TRUE = 38,
    KW_AND = 94,
    KW_AS = 125,
    KW_ASSERT = 67,
    KW_BREAK = 55,
    KW_CLASS = 75,
    KW_CONTINUE = 32,
    KW_DEF = 99,
    KW_DEL = 105,
    KW_ELIF = 89,
    KW_ELSE = 98,
    KW_EXCEPT = 70,
    KW_FINALLY = 49,
    KW_FOR = 3,
    KW_FROM = 112,
    KW_GLOBAL = 52,
    KW_IF = 18,
    KW_IMPORT = 100,
    KW_IN = 26,
    KW_IS = 31,
    KW_LAMBDA = 51,
    KW_NONLOCAL = 50,
    KW_NOT = 37,
    KW_OR = 54,
    KW_PASS = 19,
    KW_RAISE = 118,
    KW_RETURN = 6,
    KW_TRY = 69,
    KW_WHILE = 12,
    KW_WITH = 45,
    KW_YIELD = 16,
} Keyword;

#define KEYWORDS_MAX 126
size_t kw_hash(char* kw);
Keyword is_keyword(char* word);
const char* kw_to_cstr(Keyword kw);

typedef enum {
    OPERATOR_PLUS = 86,
    OPERATOR_MINUS = 90,
    OPERATOR_MULT = 84,
    OPERATOR_DIV = 94,
    OPERATOR_MOD = 74,
    OPERATOR_POW = 126,
    OPERATOR_FLOORDIV = 5,
    OPERATOR_ASSIGNMENT = 122,
    OPERATOR_PLUS_ASSIGNMENT = 11,
    OPERATOR_MINUS_ASSIGNMENT = 15,
    OPERATOR_MULT_ASSIGNMENT = 9,
    OPERATOR_DIV_ASSIGNMENT = 19,
    OPERATOR_MOD_ASSIGNMENT = 135,
    OPERATOR_FLOORDIV_ASSIGNMENT = 66,
    OPERATOR_POW_ASSIGNMENT = 51,
    OPERATOR_AND_ASSIGNMENT = 1,
    OPERATOR_OR_ASSIGNMENT = 37,
    OPERATOR_XOR_ASSIGNMENT = 113,
    OPERATOR_RSHIFT_ASSIGNMENT = 111,
    OPERATOR_LSHIFT_ASSIGNMENT = 105,
    OPERATOR_EQUAL = 47,
    OPERATOR_NOT_EQUAL = 127,
    OPERATOR_GREATER = 124,
    OPERATOR_LESS = 120,
    OPERATOR_GREATER_EQUAL = 49,
    OPERATOR_LESS_EQUAL = 45,
    OPERATOR_BITWISE_AND = 76,
    OPERATOR_BITWISE_OR = 112,
    OPERATOR_BITWISE_XOR = 52,
    OPERATOR_CONDITIONAL_IF = 40,
    OPERATOR_CONDITIONAL_ELSE = 118,
    OPERATOR_LSHIFT = 44,
    OPERATOR_RSHIFT = 50,
    OPERATOR_CALL = 71,
    OPERATOR_GET_ITEM = 2,
    OPERATOR_GET_ATTR = 14,
    OPERATOR_LOGICAL_AND = 132,
    OPERATOR_LOGICAL_OR = 64,
    OPERATOR_LOGICAL_NOT = 39,
    OPERATOR_IN = 48,
    OPERATOR_IS = 53,
    OPERATOR_NEGATIVE = 16,
    OPERATOR_BITWISE_NOT = 116,
} Operator;

#define OPERATORS_MAX 136
#define MAX_PRECEDENCE 16
extern const unsigned int PRECEDENCE_TABLE[OPERATORS_MAX];
extern const bool IS_ASSIGNMENT_OP[OPERATORS_MAX];
Operator op_from_cstr(char* op);
const char* op_to_cstr(Operator op);

#endif