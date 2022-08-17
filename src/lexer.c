#include "lexer.h"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tokens.h"

#define UNIMPLEMENTED() assert(0 && "unimplemented")

#define SYNTAX_ERROR(token, message)                                                     \
    do {                                                                                 \
        fprintf(stderr, "SYNTAX_ERROR %u:%u -- %s\n", token.line, token.col, message);   \
        exit(1);                                                                         \
    } while (0)

static bool IS_OP_TABLE[sizeof(unsigned char) * 256] = {
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

#define GETC(scanner) (scanner->current_col++, fgetc(scanner->srcfile))
#define UNGETC(c, scanner)                                                               \
    do {                                                                                 \
        scanner->current_col--;                                                          \
        ungetc(c, scanner->srcfile);                                                     \
    } while (0)

#define TOK_APPEND(tok, c)                                                               \
    do {                                                                                 \
        OVERFLOW_CHECK_TOK(tok);                                                         \
        tok.string.buffer[tok.string.length++] = c;                                      \
    } while (0)

#define SYNTAX_ASSERT(cond, tok, message)                                                \
    if (!(cond)) SYNTAX_ERROR(tok, message)

#define SCANNER_TOKENIZING_NEWLINES(scanner)                                             \
    (scanner->open_parens == 0 && scanner->open_square == 0 && scanner->open_curly == 0)

Lexer
lex_open(char* filepath)
{
    FILE* file = fopen(filepath, "r");
    if (!file) {
        fprintf(stderr, "ERROR: failed to open file -- %s\n", strerror(errno));
        exit(1);
    }
    Lexer lex = {.filepath = filepath, .scanner.srcfile = file};
    return lex;
}

void
lex_close(Lexer* lexptr)
{
    FILE* file;
    if ((file = lexptr->scanner.srcfile)) {
        fclose(file);
    }
}

static char
peek(Scanner* scanner)
{
    char c = fgetc(scanner->srcfile);
    ungetc(c, scanner->srcfile);
    return c;
}

Token
next_token(Scanner* scanner)
{
    Token tok = {0};
    char c;

    // skip whitespace
    while (tok.string.length == 0) {
        switch (c = GETC(scanner)) {
            case ' ':
                break;
            case '\t':
                break;
            case EOF:
                tok.type = TOK_EOF;
                tok.line = scanner->current_line + 1;
                tok.col = scanner->current_col;
                return tok;
            case '\n':
                if (!SCANNER_TOKENIZING_NEWLINES(scanner)) {
                    scanner->current_line++;
                    scanner->current_col = 0;
                    break;
                }
                // TODO: put this behind a compiler check or disable implicit fallthrough
                // warning
                __attribute__((fallthrough));
            default:
                TOK_APPEND(tok, c);
        }
    }

    // skip comments by advancing to newline token
    if (c == '#') {
        for (c = GETC(scanner); c != '\n' && c != EOF; c = GETC(scanner))
            ;
        if (c == EOF) {
            tok.type = TOK_EOF;
            tok.line = scanner->current_line + 1;
            tok.col = scanner->current_col;
            return tok;
        }
        // replace '#' with the newline
        tok.string.buffer[0] = c;
    }

    tok.line = scanner->current_line + 1;
    tok.col = scanner->current_col;

    // handle single char tokens
    switch (tok.string.buffer[0]) {
        case '\n':
            tok.type = TOK_NEWLINE;
            scanner->current_line++;
            scanner->current_col = 0;
            return tok;
        case '(':
            scanner->open_parens++;
            tok.type = TOK_OPEN_PARENS;
            return tok;
        case ')':
            SYNTAX_ASSERT(scanner->open_parens > 0, tok, "no matching '('");
            scanner->open_parens--;
            tok.type = TOK_CLOSE_PARENS;
            return tok;
        case '[':
            scanner->open_square++;
            tok.type = TOK_OPEN_SQUARE;
            return tok;
        case ']':
            SYNTAX_ASSERT(scanner->open_square > 0, tok, "no matching '['");
            scanner->open_square--;
            tok.type = TOK_CLOSE_SQUARE;
            return tok;
        case '{':
            scanner->open_curly++;
            tok.type = TOK_OPEN_CURLY;
            return tok;
        case '}':
            SYNTAX_ASSERT(scanner->open_curly > 0, tok, "no matching '{'");
            scanner->open_curly--;
            tok.type = TOK_CLOSE_CURLY;
            return tok;
        case ':':
            tok.type = TOK_COLON;
            return tok;
        case ',':
            tok.type = TOK_COMMA;
            return tok;
        case '.':
            if (IS_ALPHA(peek(scanner))) {
                tok.type = TOK_DOT;
                return tok;
            }
    }

    // token is the name of something (function, variable ...)
    if (IS_ALPHA(c)) {
        for (c = GETC(scanner); IS_ALPHA(c) || IS_NUMERIC(c); c = GETC(scanner))
            TOK_APPEND(tok, c);
        UNGETC(c, scanner);
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
        for (c = GETC(scanner); IS_NUMERIC(c) || c == '.' || c == '_';
             c = GETC(scanner)) {
            if (c == '_') continue;
            TOK_APPEND(tok, c);
        }
        UNGETC(c, scanner);
        tok.type = TOK_NUMBER;
    }

    // token is a string literal
    else if (c == '\'' || c == '"') {
        char opening_quote = c;
        tok.string.buffer[--tok.string.length] = '\0';  // mask off surrounding quotes
        for (c = GETC(scanner);
             c != opening_quote || tok.string.buffer[tok.string.length - 1] == '\\';
             c = GETC(scanner)) {
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
        for (c = GETC(scanner); IS_OPERATOR(c); c = GETC(scanner)) {
            TOK_APPEND(tok, c);
        }
        UNGETC(c, scanner);
        tok.operator= op_from_cstr(tok.string.buffer);
        tok.type = TOK_OPERATOR;
    }
    return tok;
}

static bool
is_end_of_statement(TokenStream* ts, bool in_square_brackets)
{
    switch (token_stream_peek_type(ts)) {
        case TOK_OPERATOR:
            return (IS_ASSIGNMENT_OP[token_stream_peek_unsafe(ts).operator]);
        case TOK_COLON:
            return (!in_square_brackets);
        case TOK_NEWLINE:
            return true;
        case TOK_KEYWORD:
            // TODO: there are some keywords that are actually operators
            // like `and` `or` etc. We should probably scan those in as ops
            // in the tokenizer
            return true;
        case TOK_EOF:
            return true;
        default:
            break;
    }
    return false;
}

static Statement
parse_statement(TokenStream* ts)
{
    Statement stmt = {0};
    do {
        Token tok = token_stream_consume(ts);
        if (tok.type == NULL_TOKEN) UNIMPLEMENTED();
        stmt.tokens[stmt.length++] = tok;
        if (stmt.length == STATEMENT_TOKEN_CAPACITY) UNIMPLEMENTED();
    } while (!is_end_of_statement(ts, false));
    return stmt;
}

static Instruction
parse_for_loop_instruction(TokenStream* ts)
{
    Instruction inst = {.type = INST_FOR_LOOP};
    // consume for token
    Token tok = token_stream_consume(ts);

    tok = token_stream_consume(ts);
    SYNTAX_ASSERT(tok.type == TOK_NAME, tok, "expected identifier token");
    inst.for_loop.it = tok.string;

    tok = token_stream_consume(ts);
    SYNTAX_ASSERT(
        tok.type == TOK_KEYWORD && tok.keyword == KW_IN, tok, "expected keyword `in`"
    );

    inst.for_loop.iterable_stmt = parse_statement(ts);

    tok = token_stream_consume(ts);
    SYNTAX_ASSERT(tok.type == TOK_COLON, tok, "expected `:`");

    return inst;
}

static Instruction
parse_unknown_instruction(TokenStream* ts)
{
    Instruction inst = {.type = NULL_INST};
    (void)token_stream_consume(ts);
    return inst;
}

Instruction
next_instruction(Lexer* lex)
{
    Instruction inst = {.type = NULL_INST};
    TokenStream* ts = &lex->scanner.ts;

    while (token_stream_peek_type(ts) == TOK_NEWLINE) (void)token_stream_consume(ts);

    Token first_token = token_stream_peek(ts);
    if (first_token.type == TOK_KEYWORD) {
        switch (first_token.keyword) {
            case KW_FOR:
                return parse_for_loop_instruction(ts);
            default:
                return parse_unknown_instruction(ts);
        }
    }
    else if (first_token.type == TOK_EOF) {
        (void)token_stream_consume(ts);
        inst.type = INST_EOF;
        return inst;
    }

    inst.type = INST_STMT;
    inst.stmt = parse_statement(ts);
    return inst;
}

void
scan_to_token_stream(Scanner* scanner)
{
    Token tok;
    do {
        tok = next_token(scanner);
        if (token_stream_write(&scanner->ts, tok)) {
            UNIMPLEMENTED();
        };
    } while (tok.type != TOK_EOF);
}
