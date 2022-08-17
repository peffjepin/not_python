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

Operator
op_from_cstr(char* op)
{
    size_t hash = op[0];
    for (size_t i = 0; op[i] != '\0'; i++) hash += op[i];
    return (Operator)hash % 70;
}

static bool
token_stream_full(TokenStream* ts)
{
    if (ts->write_head < ts->read_head) return (ts->read_head - ts->write_head == 1);
    return (ts->read_head == 0 && ts->write_head == TOKEN_STREAM_CAPACITY - 1);
}

int
token_stream_write(TokenStream* ts, Token tok)
{
    if (token_stream_full(ts)) return -1;
    ts->tokens[ts->write_head++] = tok;
    if (ts->write_head == TOKEN_STREAM_CAPACITY) ts->write_head = 0;
    return 0;
}

Token
token_stream_consume(TokenStream* ts)
{
    Token tok = {0};
    if (ts->read_head == ts->write_head) return tok;
    tok = ts->tokens[ts->read_head];
    ts->tokens[ts->read_head].type = NULL_TOKEN;
    if (ts->read_head == TOKEN_STREAM_CAPACITY - 1)
        ts->read_head = 0;
    else
        ts->read_head++;
    return tok;
}

Token
token_stream_peek(TokenStream* ts)
{
    Token null_tok = {0};
    if (ts->read_head == ts->write_head) return null_tok;
    return ts->tokens[ts->read_head];
}

Token
token_stream_peekn(TokenStream* ts, size_t n)
{
    Token null_tok = {0};
    size_t read_index = (ts->read_head + n) % TOKEN_STREAM_CAPACITY;

    // boundary checks
    if (read_index == ts->write_head) return null_tok;
    if (ts->read_head > ts->write_head && read_index > ts->write_head &&
        read_index < ts->read_head)
        return null_tok;
    else if (read_index < ts->read_head || read_index > ts->write_head)
        return null_tok;

    return ts->tokens[read_index];
}
