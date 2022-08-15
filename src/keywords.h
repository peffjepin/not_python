#ifndef KEYWORDS_H
#define KEYWORDS_H

#include <stddef.h>

typedef enum {
    NOT_A_KEYWORD = 0,
    KW_AND = 17,
    KW_AS = 67,
    KW_ASSERT = 78,
    KW_BREAK = 82,
    KW_CLASS = 99,
    KW_CONTINUE = 144,
    KW_DEF = 13,
    KW_DEL = 19,
    KW_ELIF = 126,
    KW_ELSE = 135,
    KW_EXCEPT = 69,
    KW_FINALLY = 26,
    KW_FOR = 37,
    KW_FROM = 1,
    KW_GLOBAL = 45,
    KW_IF = 62,
    KW_IMPORT = 87,
    KW_IN = 70,
    KW_IS = 75,
    KW_LAMBDA = 29,
    KW_NONLOCAL = 129,
    KW_NOT = 47,
    KW_OR = 80,
    KW_PASS = 4,
    KW_RAISE = 97,
    KW_RETURN = 92,
    KW_TRY = 61,
    KW_WHILE = 102,
    KW_WITH = 9,
    KW_YIELD = 100,
} Keyword;

// returns either NOT_A_KEYWORD or KW_*
Keyword is_keyword(char* word);

#endif
