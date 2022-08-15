#include "lexer.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static bool IS_OP_TABLE[255] = {
    ['-'] = true,
    ['!'] = true,
    ['+'] = true,
    ['*'] = true,
    ['/'] = true,
    ['|'] = true,
    ['&'] = true,
    ['='] = true,
    ['<'] = true,
    ['>'] = true,
    ['^'] = true,
    ['~'] = true,
};

#define IS_OPERATOR(c) IS_OP_TABLE[(unsigned char)(c)]
#define IS_ALPHA(c) (((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' && (c) <= 'z') || c == '_')
#define IS_NUMERIC(c) (((c) >= '0' && (c) <= '9'))

Token
next_token(Lexer* lex)
{
    ShortStr string = {0};
    Token tok = {.value = string};
    char c;
    char next;

    // skip whitespace
    while (string.length == 0) {
        switch (c = fgetc(lex->srcfile)) {
            case ' ':
                break;
            case '\t':
                break;
            case EOF:
                return tok;
            default:
                string.buffer[string.length++] = c;
        }
        lex->current_col++;
    }

    tok.line = lex->current_line + 1;
    tok.col = lex->current_col;

    // handle single char tokens
    switch (string.buffer[0]) {
        case '\n':
            tok.type = NEWLINE;
            lex->current_line++;
            lex->current_col = 0;
            return tok;
        case '(':
            tok.type = OPEN_PARENS;
            return tok;
        case ')':
            tok.type = CLOSE_PARENS;
            return tok;
        case '[':
            tok.type = OPEN_SQUARE;
            return tok;
        case ']':
            tok.type = CLOSE_SQUARE;
            return tok;
        case '{':
            tok.type = OPEN_CURLY;
            return tok;
        case '}':
            tok.type = CLOSE_CURLY;
            return tok;
        case ':':
            tok.type = COLON;
            return tok;
        case '.':
            next = fgetc(lex->srcfile);
            ungetc(next, lex->srcfile);
            if (IS_ALPHA(next)) {
                tok.type = DOT;
                return tok;
            }
    }

    // multichar tokens
    if (IS_ALPHA(c)) {
        c = fgetc(lex->srcfile);
        while (IS_ALPHA(c) || IS_NUMERIC(c)) {
            OVERFLOW_CHECK_SHORT_STR(string, lex);
            string.buffer[string.length++] = c;
            lex->current_col++;
            c = fgetc(lex->srcfile);
        }
        ungetc(c, lex->srcfile);

        Keyword kw = is_keyword(string.buffer);
        if (kw) {
            tok.keyword = kw;
            tok.type = KEYWORD;
        }
        else {
            tok.type = NAME;
        }
    }
    else if (IS_NUMERIC(c)) {
        c = fgetc(lex->srcfile);
        while (IS_NUMERIC(c) || c == '.' || c == '_') {
            OVERFLOW_CHECK_SHORT_STR(string, lex);
            if (c != '_') string.buffer[string.length++] = c;
            lex->current_col++;
            c = fgetc(lex->srcfile);
        }
        ungetc(c, lex->srcfile);
        tok.type = NUMBER;
    }
    else {
        assert(IS_OPERATOR(c));
        c = fgetc(lex->srcfile);
        while (IS_OPERATOR(c)) {
            OVERFLOW_CHECK_SHORT_STR(string, lex);
            string.buffer[string.length++] = c;
            lex->current_col++;
            c = fgetc(lex->srcfile);
        }
        ungetc(c, lex->srcfile);
        tok.operator= op_from_cstr(string.buffer);
        tok.type = OPERATOR;
    }
    return tok;
}
