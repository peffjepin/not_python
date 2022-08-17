#include "lexer.h"

#include <assert.h>
#include <errno.h>
#include <string.h>

#define UNIMPLEMENTED(msg) assert(0 && "unimplemented: " msg)

#define SYNTAX_ERROR(token, message)                                                     \
    do {                                                                                 \
        fprintf(                                                                         \
            stderr,                                                                      \
            "%s:%u:%u (SYNTAX_ERROR) %s\n",                                              \
            token.loc.filename,                                                          \
            token.loc.line,                                                              \
            token.loc.col,                                                               \
            message                                                                      \
        );                                                                               \
        exit(1);                                                                         \
    } while (0)

static bool CHAR_CHAR_IS_OPERATOR_TABLE[sizeof(unsigned char) * 256] = {
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

#define CHAR_IS_OPERATOR(c) CHAR_CHAR_IS_OPERATOR_TABLE[(unsigned char)(c)]
#define CHAR_IS_ALPHA(c)                                                                 \
    (((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' && (c) <= 'z') || c == '_')
#define CHAR_IS_NUMERIC(c) (((c) >= '0' && (c) <= '9'))

#define SCANNER_GETC(scanner_ptr) (scanner_ptr->loc.col++, fgetc(scanner_ptr->srcfile))
#define SCANNER_UNGETC(c, scanner_ptr)                                                   \
    do {                                                                                 \
        scanner_ptr->loc.col--;                                                          \
        ungetc(c, scanner_ptr->srcfile);                                                 \
    } while (0)

#define TOK_APPEND(tok, c)                                                               \
    do {                                                                                 \
        OVERFLOW_CHECK_TOK(tok);                                                         \
        tok.string.buffer[tok.string.length++] = c;                                      \
    } while (0)

#define SYNTAX_ASSERT(cond, tok, message)                                                \
    if (!(cond)) SYNTAX_ERROR(tok, message)

typedef struct {
    FILE* srcfile;
    TokenStream* ts;
    Location loc;
} Scanner;

Lexer
lexer_open(const char* filepath)
{
    FILE* file = fopen(filepath, "r");
    if (!file) {
        fprintf(stderr, "ERROR: failed to open file -- %s\n", strerror(errno));
        exit(1);
    }
    Lexer lex = {.filepath = filepath, .srcfile = file};
    return lex;
}

void
lexer_close(Lexer* lexer)
{
    if ((lexer->srcfile)) fclose(lexer->srcfile);
}

static char
scanner_peek_char(Scanner* scanner)
{
    char c = fgetc(scanner->srcfile);
    ungetc(c, scanner->srcfile);
    return c;
}

static Token
scan_token(Scanner* scanner)
{
    Token tok = {0};
    char c;

    // skip whitespace
    while (tok.string.length == 0) {
        switch (c = SCANNER_GETC(scanner)) {
            case ' ':
                break;
            case '\t':
                break;
            case EOF:
                tok.type = TOK_EOF;
                tok.loc = scanner->loc;
                return tok;
            default:
                TOK_APPEND(tok, c);
        }
    }

    // skip comments by advancing to newline token
    if (c == '#') {
        for (c = SCANNER_GETC(scanner); c != '\n' && c != EOF; c = SCANNER_GETC(scanner))
            ;
        if (c == EOF) {
            tok.type = TOK_EOF;
            tok.loc = scanner->loc;
            return tok;
        }
        // replace '#' with the newline
        tok.string.buffer[0] = c;
    }

    tok.loc = scanner->loc;

    // handle single char tokens
    switch (tok.string.buffer[0]) {
        case '\n':
            tok.type = TOK_NEWLINE;
            scanner->loc.line++;
            scanner->loc.col = 0;
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
            if (CHAR_IS_ALPHA(scanner_peek_char(scanner))) {
                tok.type = TOK_DOT;
                return tok;
            }
    }

    // token is the name of something (function, variable ...)
    if (CHAR_IS_ALPHA(c)) {
        for (c = SCANNER_GETC(scanner); CHAR_IS_ALPHA(c) || CHAR_IS_NUMERIC(c);
             c = SCANNER_GETC(scanner))
            TOK_APPEND(tok, c);
        SCANNER_UNGETC(c, scanner);
        Keyword kw;
        if ((kw = is_keyword(tok.string.buffer))) {
            tok.keyword = kw;
            tok.type = TOK_KEYWORD;
        }
        else {
            tok.type = TOK_IDENTIFIER;
        }
    }

    // token is a numeric constant
    else if (CHAR_IS_NUMERIC(c) || c == '.') {
        for (c = SCANNER_GETC(scanner); CHAR_IS_NUMERIC(c) || c == '.' || c == '_';
             c = SCANNER_GETC(scanner)) {
            if (c == '_') continue;
            TOK_APPEND(tok, c);
        }
        SCANNER_UNGETC(c, scanner);
        tok.type = TOK_NUMBER;
    }

    // token is a string literal
    else if (c == '\'' || c == '"') {
        char opening_quote = c;
        tok.string.buffer[--tok.string.length] = '\0';  // mask off surrounding quotes
        for (c = SCANNER_GETC(scanner);
             c != opening_quote || tok.string.buffer[tok.string.length - 1] == '\\';
             c = SCANNER_GETC(scanner)) {
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
        assert(CHAR_IS_OPERATOR(c));
        for (c = SCANNER_GETC(scanner); CHAR_IS_OPERATOR(c); c = SCANNER_GETC(scanner)) {
            TOK_APPEND(tok, c);
        }
        SCANNER_UNGETC(c, scanner);
        tok.operator= op_from_cstr(tok.string.buffer);
        tok.type = TOK_OPERATOR;
    }
    return tok;
}

static bool
is_end_of_expression(TokenStream* ts, bool in_square_brackets)
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

static Expression
parse_expression(TokenStream* ts)
{
    Expression expr = {0};
    do {
        Token tok = token_stream_consume(ts);
        if (tok.type == NULL_TOKEN) UNIMPLEMENTED("token stream empty");
        expr.tokens[expr.length++] = tok;
        if (expr.length == STATEMENT_TOKEN_CAPACITY)
            UNIMPLEMENTED("expression length exceeded");
    } while (!is_end_of_expression(ts, false));
    return expr;
}

static Instruction
parse_for_loop_instruction(TokenStream* ts)
{
    Instruction inst = {.type = INST_FOR_LOOP};
    // consume for token
    Token tok = token_stream_consume(ts);

    tok = token_stream_consume(ts);
    SYNTAX_ASSERT(tok.type == TOK_IDENTIFIER, tok, "expected identifier token");
    inst.for_loop.it = tok.string;

    tok = token_stream_consume(ts);
    SYNTAX_ASSERT(
        tok.type == TOK_KEYWORD && tok.keyword == KW_IN, tok, "expected keyword `in`"
    );

    inst.for_loop.iterable_expr = parse_expression(ts);

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
next_instruction(Lexer* lexer)
{
    Instruction inst = {.type = NULL_INST};

    while (token_stream_peek_type(&lexer->ts) == TOK_NEWLINE)
        (void)token_stream_consume(&lexer->ts);

    Token first_token = token_stream_peek(&lexer->ts);
    if (first_token.type == TOK_KEYWORD) {
        switch (first_token.keyword) {
            case KW_FOR:
                return parse_for_loop_instruction(&lexer->ts);
            default:
                return parse_unknown_instruction(&lexer->ts);
        }
    }
    else if (first_token.type == TOK_EOF) {
        (void)token_stream_consume(&lexer->ts);
        inst.type = INST_EOF;
        return inst;
    }

    inst.type = INST_EXPR;
    inst.expr = parse_expression(&lexer->ts);
    return inst;
}

void
lexer_tokenize(Lexer* lexer)
{
    Scanner scanner = {
        .loc.line = 1,
        .loc.filename = lexer->filepath,
        .srcfile = lexer->srcfile,
        .ts = &lexer->ts};
    Token tok;
    do {
        tok = scan_token(&scanner);
        if (token_stream_write(scanner.ts, tok)) {
            UNIMPLEMENTED("token stream full");
        };
    } while (tok.type != TOK_EOF);
}
