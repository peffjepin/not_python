#include "operators.h"

#include <stddef.h>

const bool IS_ASSIGNMENT_OP[70] = {
    [OPERATOR_ASSIGNMENT] = true,
    [OPERATOR_PLUS_ASSIGNMENT] = true,
    [OPERATOR_MINUS_ASSIGNMENT] = true,
    [OPERATOR_MULT_ASSIGNMENT] = true,
    [OPERATOR_DIV_ASSIGNMENT] = true,
    [OPERATOR_MOD_ASSIGNMENT] = true,
    [OPERATOR_FLOORDIV_ASSIGNMENT] = true,
    [OPERATOR_POW_ASSIGNMENT] = true,
    [OPERATOR_AND_ASSIGNMENT] = true,
    [OPERATOR_OR_ASSIGNMENT] = true,
    [OPERATOR_XOR_ASSIGNMENT] = true,
    [OPERATOR_RSHIFT_ASSIGNMENT] = true,
    [OPERATOR_LSHIFT_ASSIGNMENT] = true,
};

Operator
op_from_cstr(char* op)
{
    size_t hash = op[0];
    for (size_t i = 0; op[i] != '\0'; i++) hash += op[i];
    return (Operator)hash % 70;
}
