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

#define TOK_APPEND(tok, c)                                                               \
    do {                                                                                 \
        OVERFLOW_CHECK_TOK(tok);                                                         \
        tok.string.buffer[tok.string.length++] = c;                                      \
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
    Token tok = {0};
    char c;

    // skip whitespace
    while (tok.string.length == 0) {
        switch (c = GETC(lex)) {
            case ' ':
                break;
            case '\t':
                break;
            case EOF:
                return tok;
            default:
                TOK_APPEND(tok, c);
        }
    }

    // skip comments by advancing to newline token
    if (c == '#') {
        for (c = GETC(lex); c != '\n' && c != EOF; c = GETC(lex))
            ;
        if (c == EOF) {
            return tok;
        }
        // replace '#' with the newline
        tok.string.buffer[0] = c;
    }

    tok.line = lex->current_line + 1;
    tok.col = lex->current_col;

    // handle single char tokens
    switch (tok.string.buffer[0]) {
        case '\n':
            tok.type = TOK_NEWLINE;
            lex->current_line++;
            lex->current_col = 0;
            return tok;
        case '(':
            tok.type = TOK_OPEN_PARENS;
            return tok;
        case ')':
            tok.type = TOK_CLOSE_PARENS;
            return tok;
        case '[':
            tok.type = TOK_OPEN_SQUARE;
            return tok;
        case ']':
            tok.type = TOK_CLOSE_SQUARE;
            return tok;
        case '{':
            tok.type = TOK_OPEN_CURLY;
            return tok;
        case '}':
            tok.type = TOK_CLOSE_CURLY;
            return tok;
        case ':':
            tok.type = TOK_COLON;
            return tok;
        case ',':
            tok.type = TOK_COMMA;
            return tok;
        case '.':
            if (IS_ALPHA(peek(lex))) {
                tok.type = TOK_DOT;
                return tok;
            }
    }

    // token is the name of something (function, variable ...)
    if (IS_ALPHA(c)) {
        for (c = GETC(lex); IS_ALPHA(c) || IS_NUMERIC(c); c = GETC(lex))
            TOK_APPEND(tok, c);
        UNGETC(c, lex);
        Keyword kw;
        if ((kw = is_keyword(tok.string.buffer))) {
            tok.keyword = kw;
            tok.type = TOK_KEYWORD;
        }
        else {
            tok.type = TOK_NAME;
        }
    }

    // token is a numeric constant
    else if (IS_NUMERIC(c) || c == '.') {
        for (c = GETC(lex); IS_NUMERIC(c) || c == '.' || c == '_'; c = GETC(lex)) {
            if (c == '_') continue;
            TOK_APPEND(tok, c);
        }
        UNGETC(c, lex);
        tok.type = TOK_NUMBER;
    }

    // token is a string literal
    else if (c == '\'' || c == '"') {
        char opening_quote = c;
        tok.string.buffer[--tok.string.length] = '\0';  // mask off surrounding quotes
        for (c = GETC(lex);
             c != opening_quote || tok.string.buffer[tok.string.length - 1] == '\\';
             c = GETC(lex)) {
            if (c == EOF) SYNTAX_ERROR(tok, "unterminated string literal");
            if (c == opening_quote)
                tok.string.buffer[tok.string.length - 1] = c;  // overwrite '\'
            else
                TOK_APPEND(tok, c);
        }
        tok.type = TOK_STRING;
    }

    // token is an operator
    else {
        assert(IS_OPERATOR(c));
        for (c = GETC(lex); IS_OPERATOR(c); c = GETC(lex)) {
            TOK_APPEND(tok, c);
        }
        UNGETC(c, lex);
        tok.operator= op_from_cstr(tok.string.buffer);
        tok.type = TOK_OPERATOR;
    }
    return tok;
}
