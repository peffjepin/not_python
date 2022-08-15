#ifndef OPERATORS_H
#define OPERATORS_H

typedef enum {
    PLUS = 16,
    MINUS = 20,
    MULT = 14,
    DIV = 24,
    MOD = 4,
    POW = 56,
    FLOORDIV = 1,
    ASSIGNMENT = 52,
    PLUS_ASSIGNMENT = 7,
    MINUS_ASSIGNMENT = 11,
    MULT_ASSIGNMENT = 5,
    DIV_ASSIGNMENT = 15,
    MOD_ASSIGNMENT = 65,
    FLOORDIV_ASSIGNMENT = 62,
    POW_ASSIGNMENT = 47,
    AND_ASSIGNMENT = 67,
    OR_ASSIGNMENT = 29,
    XOR_ASSIGNMENT = 39,
    RSHIFT_ASSIGNMENT = 37,
    LSHIFT_ASSIGNMENT = 31,
    EQUAL = 43,
    NOT_EQUAL = 57,
    GREATER = 54,
    LESS = 50,
    GREATER_EQUAL = 45,
    LESS_EQUAL = 41,
    BITWISE_AND = 6,
    BITWISE_OR = 38,
    BITWISE_XOR = 48,
    BITWISE_NOT = 42,
    LSHIFT = 40,
    RSHIFT = 46,
} Operator;

Operator op_from_cstr(char* op);

#endif
