#include "keywords.h"

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
