#include "lexer.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define SYNTAX_ERROR(token, message)                                                     \
    do {                                                                                 \
        fprintf(stderr, "SYNTAX_ERROR %u:%u -- %s\n", token.line, token.col, message);   \
        exit(1);                                                                         \
    } while (0)

static bool IS_OP_TABLE[255] = {
    ['-'] = true,
    ['!'] = true,
    ['+'] = true,
    ['*'] = true,
    ['/'] = true,
    ['|'] = true,
    ['&'] = true,
    ['%'] = true,
    ['='] = true,
    ['<'] = true,
    ['>'] = true,
    ['^'] = true,
    ['~'] = true,
};

#define IS_OPERATOR(c) IS_OP_TABLE[(unsigned char)(c)]
#define IS_ALPHA(c) (((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' && (c) <= 'z') || c == '_')
#define IS_NUMERIC(c) (((c) >= '0' && (c) <= '9'))

#define GETC(lex_ptr) (lex_ptr->current_col++, fgetc(lex_ptr->srcfile))
#define UNGETC(c, lex_ptr)                                                               \
    do {                                                                                 \
        lex_ptr->current_col--;                                                          \
        ungetc(c, lex_ptr->srcfile);                                                     \
    } while (0)

static char
peek(Lexer* lex)
{
    char c = fgetc(lex->srcfile);
    ungetc(c, lex->srcfile);
    return c;
}

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

    // skip comments by advancing to newline token
    if (c == '#') {
        do {
            c = fgetc(lex->srcfile);
            lex->current_col++;
        } while (c != '\n' && c != EOF);
        if (c == EOF) {
            return tok;
        }
        string.buffer[0] = c;
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
        case ',':
            tok.type = COMMA;
            return tok;
        case '.':
            next = peek(lex);
            if (IS_ALPHA(next)) {
                tok.type = DOT;
                return tok;
            }
    }

    // token is the name of something (function, variable ...)
    if (IS_ALPHA(c)) {
        for (c = GETC(lex); IS_ALPHA(c) || IS_NUMERIC(c); c = GETC(lex)) {
            OVERFLOW_CHECK_SHORT_STR(string, lex);
            string.buffer[string.length++] = c;
        }
        UNGETC(c, lex);
        Keyword kw = is_keyword(string.buffer);
        if (kw) {
            tok.keyword = kw;
            tok.type = KEYWORD;
        }
        else {
            tok.value = string;
            tok.type = NAME;
        }
    }

    // token is a numeric constant
    else if (IS_NUMERIC(c) || c == '.') {
        for (c = GETC(lex); IS_NUMERIC(c) || c == '.' || c == '_'; c = GETC(lex)) {
            OVERFLOW_CHECK_SHORT_STR(string, lex);
            if (c != '_') string.buffer[string.length++] = c;
        }
        UNGETC(c, lex);
        tok.type = NUMBER;
        tok.value = string;
    }

    // token is a string literal
    else if (c == '\'' || c == '"') {
        char opening_quote = c;
        string.buffer[0] = '\0';
        string.length = 0;
        for (c = GETC(lex);
             c != opening_quote || string.buffer[string.length - 1] == '\\';
             c = GETC(lex)) {
            if (c == EOF) SYNTAX_ERROR(tok, "unterminated string literal");
            if (c == opening_quote)
                string.buffer[string.length - 1] = c;  // overwrite '\'
            else
                string.buffer[string.length++] = c;
        }
        tok.value = string;
        tok.type = STRING;
    }

    // token is an operator
    else {
        assert(IS_OPERATOR(c));
        for (c = GETC(lex); IS_OPERATOR(c); c = GETC(lex)) {
            OVERFLOW_CHECK_SHORT_STR(string, lex);
            string.buffer[string.length++] = c;
        }
        UNGETC(c, lex);
        tok.operator= op_from_cstr(string.buffer);
        tok.type = OPERATOR;
    }
    return tok;
}
