#include "operators.h"

#include <stddef.h>

const bool IS_ASSIGNMENT_OP[70] = {
    [OP_ASSIGNMENT] = 52,
    [OP_PLUS_ASSIGNMENT] = 7,
    [OP_MINUS_ASSIGNMENT] = 11,
    [OP_MULT_ASSIGNMENT] = 5,
    [OP_DIV_ASSIGNMENT] = 15,
    [OP_MOD_ASSIGNMENT] = 65,
    [OP_FLOORDIV_ASSIGNMENT] = 62,
    [OP_POW_ASSIGNMENT] = 47,
    [OP_AND_ASSIGNMENT] = 67,
    [OP_OR_ASSIGNMENT] = 29,
    [OP_XOR_ASSIGNMENT] = 39,
    [OP_RSHIFT_ASSIGNMENT] = 37,
    [OP_LSHIFT_ASSIGNMENT] = 31,
};

Operator
op_from_cstr(char* op)
{
    size_t hash = op[0];
    for (size_t i = 0; op[i] != '\0'; i++) hash += op[i];
    return (Operator)hash % 70;
}
