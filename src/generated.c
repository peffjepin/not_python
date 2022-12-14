/*
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
this code is generated by `scripts/codegen.py` and should not be edited manually
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*/
#include "generated.h"

#include <string.h>

static const char* kw_hashtable[KEYWORDS_MAX] = {
    [KW_FALSE] = "False",
    [KW_TRUE] = "True",
    [KW_AND] = "and",
    [KW_AS] = "as",
    [KW_ASSERT] = "assert",
    [KW_BREAK] = "break",
    [KW_CLASS] = "class",
    [KW_CONTINUE] = "continue",
    [KW_DEF] = "def",
    [KW_DEL] = "del",
    [KW_ELIF] = "elif",
    [KW_ELSE] = "else",
    [KW_EXCEPT] = "except",
    [KW_FINALLY] = "finally",
    [KW_FOR] = "for",
    [KW_FROM] = "from",
    [KW_GLOBAL] = "global",
    [KW_IF] = "if",
    [KW_IMPORT] = "import",
    [KW_IN] = "in",
    [KW_IS] = "is",
    [KW_LAMBDA] = "lambda",
    [KW_NONLOCAL] = "nonlocal",
    [KW_NOT] = "not",
    [KW_OR] = "or",
    [KW_PASS] = "pass",
    [KW_RAISE] = "raise",
    [KW_RETURN] = "return",
    [KW_TRY] = "try",
    [KW_WHILE] = "while",
    [KW_WITH] = "with",
    [KW_YIELD] = "yield",
};

size_t
kw_hash(char* kw)
{
    size_t hash = kw[0] * 3;
    for (size_t i = 0; kw[i] != '\0'; i++) hash += kw[i];
    return hash % KEYWORDS_MAX;
}

Keyword
is_keyword(char* word)
{
    if (!word) return NOT_A_KEYWORD;
    size_t hash = kw_hash(word);
    const char* kw = kw_hashtable[hash];
    if (!kw) return NOT_A_KEYWORD;
    if (strcmp(word, kw_hashtable[hash]) == 0) return hash;
    return NOT_A_KEYWORD;
}

const char*
kw_to_cstr(Keyword kw)
{
    return kw_hashtable[kw];
}

const unsigned int PRECEDENCE_TABLE[OPERATORS_MAX] = {
    [OPERATOR_PLUS] = 11,
    [OPERATOR_MINUS] = 11,
    [OPERATOR_MULT] = 12,
    [OPERATOR_DIV] = 12,
    [OPERATOR_MOD] = 12,
    [OPERATOR_POW] = 14,
    [OPERATOR_FLOORDIV] = 12,
    [OPERATOR_ASSIGNMENT] = 0,
    [OPERATOR_PLUS_ASSIGNMENT] = 0,
    [OPERATOR_MINUS_ASSIGNMENT] = 0,
    [OPERATOR_MULT_ASSIGNMENT] = 0,
    [OPERATOR_DIV_ASSIGNMENT] = 0,
    [OPERATOR_MOD_ASSIGNMENT] = 0,
    [OPERATOR_FLOORDIV_ASSIGNMENT] = 0,
    [OPERATOR_POW_ASSIGNMENT] = 0,
    [OPERATOR_AND_ASSIGNMENT] = 0,
    [OPERATOR_OR_ASSIGNMENT] = 0,
    [OPERATOR_XOR_ASSIGNMENT] = 0,
    [OPERATOR_RSHIFT_ASSIGNMENT] = 0,
    [OPERATOR_LSHIFT_ASSIGNMENT] = 0,
    [OPERATOR_EQUAL] = 6,
    [OPERATOR_NOT_EQUAL] = 6,
    [OPERATOR_GREATER] = 6,
    [OPERATOR_LESS] = 6,
    [OPERATOR_GREATER_EQUAL] = 6,
    [OPERATOR_LESS_EQUAL] = 6,
    [OPERATOR_BITWISE_AND] = 9,
    [OPERATOR_BITWISE_OR] = 7,
    [OPERATOR_BITWISE_XOR] = 8,
    [OPERATOR_CONDITIONAL_IF] = 2,
    [OPERATOR_CONDITIONAL_ELSE] = 2,
    [OPERATOR_LSHIFT] = 10,
    [OPERATOR_RSHIFT] = 10,
    [OPERATOR_CALL] = 16,
    [OPERATOR_GET_ITEM] = 16,
    [OPERATOR_GET_ATTR] = 16,
    [OPERATOR_LOGICAL_AND] = 4,
    [OPERATOR_LOGICAL_OR] = 3,
    [OPERATOR_LOGICAL_NOT] = 5,
    [OPERATOR_IN] = 6,
    [OPERATOR_IS] = 6,
    [OPERATOR_NEGATIVE] = 13,
    [OPERATOR_BITWISE_NOT] = 13,
};

const bool IS_ASSIGNMENT_OP[OPERATORS_MAX] = {
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

static const char* OP_TO_CSTR_TABLE[OPERATORS_MAX] = {
    [OPERATOR_PLUS] = "+",
    [OPERATOR_MINUS] = "-",
    [OPERATOR_MULT] = "*",
    [OPERATOR_DIV] = "/",
    [OPERATOR_MOD] = "%",
    [OPERATOR_POW] = "**",
    [OPERATOR_FLOORDIV] = "//",
    [OPERATOR_ASSIGNMENT] = "=",
    [OPERATOR_PLUS_ASSIGNMENT] = "+=",
    [OPERATOR_MINUS_ASSIGNMENT] = "-=",
    [OPERATOR_MULT_ASSIGNMENT] = "*=",
    [OPERATOR_DIV_ASSIGNMENT] = "/=",
    [OPERATOR_MOD_ASSIGNMENT] = "%=",
    [OPERATOR_FLOORDIV_ASSIGNMENT] = "//=",
    [OPERATOR_POW_ASSIGNMENT] = "**=",
    [OPERATOR_AND_ASSIGNMENT] = "&=",
    [OPERATOR_OR_ASSIGNMENT] = "|=",
    [OPERATOR_XOR_ASSIGNMENT] = "^=",
    [OPERATOR_RSHIFT_ASSIGNMENT] = ">>=",
    [OPERATOR_LSHIFT_ASSIGNMENT] = "<<=",
    [OPERATOR_EQUAL] = "==",
    [OPERATOR_NOT_EQUAL] = "!=",
    [OPERATOR_GREATER] = ">",
    [OPERATOR_LESS] = "<",
    [OPERATOR_GREATER_EQUAL] = ">=",
    [OPERATOR_LESS_EQUAL] = "<=",
    [OPERATOR_BITWISE_AND] = "&",
    [OPERATOR_BITWISE_OR] = "|",
    [OPERATOR_BITWISE_XOR] = "^",
    [OPERATOR_CONDITIONAL_IF] = "if",
    [OPERATOR_CONDITIONAL_ELSE] = "else",
    [OPERATOR_LSHIFT] = "<<",
    [OPERATOR_RSHIFT] = ">>",
    [OPERATOR_CALL] = "__call__",
    [OPERATOR_GET_ITEM] = "__getitem__",
    [OPERATOR_GET_ATTR] = "__getattr__",
    [OPERATOR_LOGICAL_AND] = "and",
    [OPERATOR_LOGICAL_OR] = "or",
    [OPERATOR_LOGICAL_NOT] = "not",
    [OPERATOR_IN] = "in",
    [OPERATOR_IS] = "is",
    [OPERATOR_NEGATIVE] = "neg",
    [OPERATOR_BITWISE_NOT] = "~",
};

Operator
op_from_cstr(char* op)
{
    size_t hash = op[0];
    for (size_t i = 0; op[i] != '\0'; i++) hash += op[i];
    return (Operator)hash % OPERATORS_MAX;
}

const char*
op_to_cstr(Operator op)
{
    return OP_TO_CSTR_TABLE[op];
}
