#include "operators.h"

#include <stddef.h>

const bool IS_ASSIGNMENT_OP[70] = {
    [OP_ASSIGNMENT] = true,
    [OP_PLUS_ASSIGNMENT] = true,
    [OP_MINUS_ASSIGNMENT] = true,
    [OP_MULT_ASSIGNMENT] = true,
    [OP_DIV_ASSIGNMENT] = true,
    [OP_MOD_ASSIGNMENT] = true,
    [OP_FLOORDIV_ASSIGNMENT] = true,
    [OP_POW_ASSIGNMENT] = true,
    [OP_AND_ASSIGNMENT] = true,
    [OP_OR_ASSIGNMENT] = true,
    [OP_XOR_ASSIGNMENT] = true,
    [OP_RSHIFT_ASSIGNMENT] = true,
    [OP_LSHIFT_ASSIGNMENT] = true,
};

Operator
op_from_cstr(char* op)
{
    size_t hash = op[0];
    for (size_t i = 0; op[i] != '\0'; i++) hash += op[i];
    return (Operator)hash % 70;
}
