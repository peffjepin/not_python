#include "lexer_types.h"

#include <string.h>

static const char* kw_hashtable[145] = {
    [KW_AND] = "and",     [KW_AS] = "as",         [KW_ASSERT] = "assert",
    [KW_BREAK] = "break", [KW_CLASS] = "class",   [KW_CONTINUE] = "continue",
    [KW_DEF] = "def",     [KW_DEL] = "del",       [KW_ELIF] = "elif",
    [KW_ELSE] = "else",   [KW_EXCEPT] = "except", [KW_FINALLY] = "finally",
    [KW_FOR] = "for",     [KW_FROM] = "from",     [KW_GLOBAL] = "global",
    [KW_IF] = "if",       [KW_IMPORT] = "import", [KW_IN] = "in",
    [KW_IS] = "is",       [KW_LAMBDA] = "lambda", [KW_NONLOCAL] = "nonlocal",
    [KW_NOT] = "not",     [KW_OR] = "or",         [KW_PASS] = "pass",
    [KW_RAISE] = "raise", [KW_RETURN] = "return", [KW_TRY] = "try",
    [KW_WHILE] = "while", [KW_WITH] = "with",     [KW_YIELD] = "yield",
};

static size_t
kw_hash(char* kw)
{
    size_t hash = 0;
    for (size_t i = 0; kw[i] != '\0'; i++) hash += kw[i];
    return hash % 145;
}

Keyword
is_keyword(char* word)
{
    if (!word) return NOT_A_KEYWORD;
    size_t hash = kw_hash(word);
    const char* kw = kw_hashtable[hash];
    if (!kw) return NOT_A_KEYWORD;
    if (strcmp(word, kw) == 0) return hash;
    return NOT_A_KEYWORD;
}

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

const unsigned int PRECENDENCE_TABLE[70] = {
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
    [OPERATOR_BITWISE_NOT] = 0,
    [OPERATOR_LSHIFT] = 10,
    [OPERATOR_RSHIFT] = 10,
};

Operator
op_from_cstr(char* op)
{
    size_t hash = op[0];
    for (size_t i = 0; op[i] != '\0'; i++) hash += op[i];
    return (Operator)hash % 70;
}
