#include "lexer_types.h"

#include <assert.h>
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

const char*
kw_to_cstr(Keyword kw)
{
    return kw_hashtable[kw];
}

const bool IS_ASSIGNMENT_OP[OPERATOR_TABLE_MAX] = {
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

const unsigned int PRECENDENCE_TABLE[OPERATOR_TABLE_MAX] = {
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
    [OPERATOR_CALL] = 16,
    [OPERATOR_GET_ITEM] = 16,
    [OPERATOR_GET_ATTR] = 16,
};

const char* OP_TO_CSTR_TABLE[OPERATOR_TABLE_MAX] = {
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
    [OPERATOR_BITWISE_NOT] = "~",
    [OPERATOR_LSHIFT] = "<<",
    [OPERATOR_RSHIFT] = ">>",
    [OPERATOR_CALL] = "__call__",
    [OPERATOR_GET_ITEM] = "__getitem__",
    [OPERATOR_GET_ATTR] = "__getattr__",
};

Operator
op_from_cstr(char* op)
{
    size_t hash = op[0];
    for (size_t i = 0; op[i] != '\0'; i++) hash += op[i];
    return (Operator)hash % OPERATOR_TABLE_MAX;
}

const char*
op_to_cstr(Operator op)
{
    return OP_TO_CSTR_TABLE[op];
}

const char* TOKEN_TYPE_TO_CSTR_TABLE[19] = {
    [TOK_KEYWORD] = "keyword",
    [TOK_COMMA] = ",",
    [TOK_COLON] = ":",
    [TOK_STRING] = "string",
    [TOK_NUMBER] = "number",
    [TOK_OPERATOR] = "operator",
    [TOK_NEWLINE] = "\n",
    [TOK_BLOCK_BEGIN] = "block begin",
    [TOK_BLOCK_END] = "block end",
    [TOK_OPEN_PARENS] = "(",
    [TOK_CLOSE_PARENS] = ")",
    [TOK_OPEN_SQUARE] = "[",
    [TOK_CLOSE_SQUARE] = "]",
    [TOK_OPEN_CURLY] = "{",
    [TOK_CLOSE_CURLY] = "}",
    [TOK_IDENTIFIER] = "identifier",
    [TOK_DOT] = ".",
    [TOK_EOF] = "EOF",
};

const char*
token_type_to_cstr(TokenType type)
{
    assert(type < 19 && "token type not in table");
    return TOKEN_TYPE_TO_CSTR_TABLE[type];
}
