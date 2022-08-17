#ifndef OPERATORS_H
#define OPERATORS_H

#include <stdbool.h>

typedef enum {
    OP_PLUS = 16,
    OP_MINUS = 20,
    OP_MULT = 14,
    OP_DIV = 24,
    OP_MOD = 4,
    OP_POW = 56,
    OP_FLOORDIV = 1,
    OP_ASSIGNMENT = 52,
    OP_PLUS_ASSIGNMENT = 7,
    OP_MINUS_ASSIGNMENT = 11,
    OP_MULT_ASSIGNMENT = 5,
    OP_DIV_ASSIGNMENT = 15,
    OP_MOD_ASSIGNMENT = 65,
    OP_FLOORDIV_ASSIGNMENT = 62,
    OP_POW_ASSIGNMENT = 47,
    OP_AND_ASSIGNMENT = 67,
    OP_OR_ASSIGNMENT = 29,
    OP_XOR_ASSIGNMENT = 39,
    OP_RSHIFT_ASSIGNMENT = 37,
    OP_LSHIFT_ASSIGNMENT = 31,
    OP_EQUAL = 43,
    OP_NOT_EQUAL = 57,
    OP_GREATER = 54,
    OP_LESS = 50,
    OP_GREATER_EQUAL = 45,
    OP_LESS_EQUAL = 41,
    OP_BITWISE_AND = 6,
    OP_BITWISE_OR = 38,
    OP_BITWISE_XOR = 48,
    OP_BITWISE_NOT = 42,
    OP_LSHIFT = 40,
    OP_RSHIFT = 46,
} Operator;

extern const bool IS_ASSIGNMENT_OP[70];
Operator op_from_cstr(char* op);

#endif
