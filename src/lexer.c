#include "lexer.h"

#include <assert.h>
#include <errno.h>
#include <string.h>

#define UNIMPLEMENTED(msg) assert(0 && msg)

#define SYNTAX_ERROR(loc, msg)                                                           \
    do {                                                                                 \
        fprintf(                                                                         \
            stderr,                                                                      \
            "%s:%u:%u SYNTAX ERROR: %s\n",                                               \
            (loc).filename,                                                              \
            (loc).line,                                                                  \
            (loc).col,                                                                   \
            msg                                                                          \
        );                                                                               \
        exit(1);                                                                         \
    } while (0)

#define SYNTAX_ERRORF(loc, msg, ...)                                                     \
    do {                                                                                 \
        fprintf(                                                                         \
            stderr,                                                                      \
            "%s:%u:%u SYNTAX ERROR: " msg "\n",                                          \
            (loc).filename,                                                              \
            (loc).line,                                                                  \
            (loc).col,                                                                   \
            __VA_ARGS__                                                                  \
        );                                                                               \
        exit(1);                                                                         \
    } while (0)

typedef struct {
    FILE* srcfile;
    TokenStream* ts;
    Token token;
    char c;
    Location loc;
    bool finished;
} Scanner;

static inline void
scanner_getc(Scanner* scanner)
{
    scanner->c = fgetc(scanner->srcfile);
    scanner->loc.col++;
}

static inline void
scanner_ungetc(Scanner* scanner)
{
    scanner->loc.col--;
    ungetc(scanner->c, scanner->srcfile);
}

static inline char
scanner_peek_char(Scanner* scanner)
{
    char c = fgetc(scanner->srcfile);
    ungetc(c, scanner->srcfile);
    return c;
}

static inline void
scanner_write_token(Scanner* scanner)
{
    if (token_stream_write(scanner->ts, scanner->token)) {
        UNIMPLEMENTED("token stream full");
    };
}

static bool CHAR_IS_OPERATOR_TABLE[sizeof(unsigned char) * 256] = {
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

#define CHAR_IS_OPERATOR(c) CHAR_IS_OPERATOR_TABLE[(unsigned char)(c)]
#define CHAR_IS_ALPHA(c)                                                                 \
    (((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' && (c) <= 'z') || c == '_')
#define CHAR_IS_NUMERIC(c) (((c) >= '0' && (c) <= '9'))

#define TOKEN_APPEND_CHAR(tok, c)                                                        \
    do {                                                                                 \
        OVERFLOW_CHECK_TOK(tok);                                                         \
        tok.string.buffer[tok.string.length++] = c;                                      \
    } while (0)

static inline void
scanner_skip_whitespace(Scanner* scanner)
{
    while (scanner->token.string.length == 0) {
        scanner_getc(scanner);
        switch (scanner->c) {
            case ' ':
                break;
            case '\t':
                break;
            case EOF:
                scanner->token.type = TOK_EOF;
                scanner->token.loc = scanner->loc;
                scanner->finished = true;
                scanner_write_token(scanner);
                return;
            default:
                TOKEN_APPEND_CHAR(scanner->token, scanner->c);
        }
    }
}

static inline void
scanner_skip_comments(Scanner* scanner)
{
    // skip comments by advancing to newline token
    if (scanner->c == '#') {
        for (scanner_getc(scanner); scanner->c != '\n' && scanner->c != EOF;
             scanner_getc(scanner))
            ;
        if (scanner->c == EOF) {
            scanner->token.type = TOK_EOF;
            scanner->token.loc = scanner->loc;
            scanner->finished = true;
            scanner_write_token(scanner);
            return;
        }
        // replace '#' with the newline
        scanner->c = '\n';
    }
}

static inline bool
scanner_handle_single_char_tokens(Scanner* scanner)
{
    switch (scanner->c) {
        case '\n':
            scanner->token.type = TOK_NEWLINE;
            scanner->loc.line++;
            scanner->loc.col = 0;
            scanner_write_token(scanner);
            return true;
        case '(':
            scanner->token.type = TOK_OPEN_PARENS;
            scanner_write_token(scanner);
            return true;
        case ')':
            scanner->token.type = TOK_CLOSE_PARENS;
            scanner_write_token(scanner);
            return true;
        case '[':
            scanner->token.type = TOK_OPEN_SQUARE;
            scanner_write_token(scanner);
            return true;
        case ']':
            scanner->token.type = TOK_CLOSE_SQUARE;
            scanner_write_token(scanner);
            return true;
        case '{':
            scanner->token.type = TOK_OPEN_CURLY;
            scanner_write_token(scanner);
            return true;
        case '}':
            scanner->token.type = TOK_CLOSE_CURLY;
            scanner_write_token(scanner);
            return true;
        case ':':
            scanner->token.type = TOK_COLON;
            scanner_write_token(scanner);
            return true;
        case ',':
            scanner->token.type = TOK_COMMA;
            scanner_write_token(scanner);
            return true;
        case '.':
            if (CHAR_IS_ALPHA(scanner_peek_char(scanner))) {
                scanner->token.type = TOK_DOT;
                scanner_write_token(scanner);
                return true;
            }
            return false;
        default:
            return false;
    }
    return false;
}

static inline void
scanner_tokenize_word(Scanner* scanner)
{
    for (scanner_getc(scanner); CHAR_IS_ALPHA(scanner->c) || CHAR_IS_NUMERIC(scanner->c);
         scanner_getc(scanner))
        TOKEN_APPEND_CHAR(scanner->token, scanner->c);
    scanner_ungetc(scanner);
    Keyword kw;
    if ((kw = is_keyword(scanner->token.string.buffer))) {
        scanner->token.keyword = kw;
        scanner->token.type = TOK_KEYWORD;
    }
    else {
        scanner->token.type = TOK_IDENTIFIER;
    }
}

static inline void
scanner_tokenize_numeric(Scanner* scanner)
{
    for (scanner_getc(scanner);
         CHAR_IS_NUMERIC(scanner->c) || scanner->c == '.' || scanner->c == '_';
         scanner_getc(scanner)) {
        if (scanner->c == '_') continue;
        TOKEN_APPEND_CHAR(scanner->token, scanner->c);
    }
    scanner_ungetc(scanner);
    scanner->token.type = TOK_NUMBER;
}

static inline void
scanner_tokenize_string_literal(Scanner* scanner)
{
    char opening_quote = scanner->c;
    scanner->token.string.buffer[--(scanner->token.string.length)] =
        '\0';  // mask off surrounding quotes
    for (scanner_getc(scanner);
         scanner->c != opening_quote ||
         scanner->token.string.buffer[scanner->token.string.length - 1] == '\\';
         scanner_getc(scanner)) {
        if (scanner->c == EOF)
            SYNTAX_ERROR(scanner->token.loc, "unterminated string literal");
        if (scanner->c == opening_quote)
            scanner->token.string.buffer[scanner->token.string.length - 1] =
                scanner->c;  // overwrite '\'
        else
            TOKEN_APPEND_CHAR(scanner->token, scanner->c);
    }
    scanner->token.type = TOK_STRING;
}

static inline void
scanner_tokenize_operator(Scanner* scanner)
{
    assert(CHAR_IS_OPERATOR(scanner->c));
    for (scanner_getc(scanner); CHAR_IS_OPERATOR(scanner->c); scanner_getc(scanner)) {
        TOKEN_APPEND_CHAR(scanner->token, scanner->c);
    }
    scanner_ungetc(scanner);
    scanner->token.operator= op_from_cstr(scanner->token.string.buffer);
    scanner->token.type = TOK_OPERATOR;
}

static void
scan_token(Scanner* scanner)
{
    memset(&scanner->token, 0, sizeof(Token));

    scanner_skip_whitespace(scanner);
    if (scanner->finished) return;

    scanner_skip_comments(scanner);
    if (scanner->finished) return;

    // we're now at the start of a new token
    scanner->token.loc = scanner->loc;

    if (scanner_handle_single_char_tokens(scanner))
        ;
    else if (CHAR_IS_ALPHA(scanner->c))
        scanner_tokenize_word(scanner);
    else if (CHAR_IS_NUMERIC(scanner->c) || scanner->c == '.')
        scanner_tokenize_numeric(scanner);
    else if (scanner->c == '\'' || scanner->c == '"')
        scanner_tokenize_string_literal(scanner);
    else
        scanner_tokenize_operator(scanner);
    scanner_write_token(scanner);
}

typedef struct {
    TokenStream* ts;
    unsigned int open_parens;
    unsigned int open_square;
    unsigned int open_curly;
} Parser;

static bool
parser_is_end_of_expression(Parser* parser)
{
    switch (token_stream_peek_type(parser->ts)) {
        case TOK_OPERATOR:
            return (IS_ASSIGNMENT_OP[token_stream_peek_unsafe(parser->ts).operator]);
        case TOK_COLON:
            return (parser->open_square == 0);
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

#define INTERMEDIATE_TABLE_MAX EXPRESSION_CAPACITY

typedef struct {
    Operation operations[INTERMEDIATE_TABLE_MAX];
    size_t length;
} IntermediateOperationTableEntry;

static Expression
parse_expression(Parser* parser)
{
    const unsigned int POW_PREC = PRECENDENCE_TABLE[OPERATOR_POW];
    IntermediateOperationTableEntry by_prec[MAX_PRECEDENCE + 1] = {0};
    Expression expr = {0};
    size_t operand_index = 0;

    do {
        Token tok = token_stream_consume(parser->ts);
        if (tok.type == NULL_TOKEN) UNIMPLEMENTED("token stream empty");
        switch (tok.type) {
            case TOK_OPERATOR: {
                // FIXME: the right side operand is yet to be parsed so we
                // need to add some assertation that it will be parsed
                unsigned int prec = PRECENDENCE_TABLE[tok.operator];
                Operation* operation = &by_prec[prec].operations[by_prec[prec].length++];
                operation->left = operand_index - 1;
                operation->right = operand_index;
                operation->op_type = tok.operator;
                break;
            }
            case TOK_NUMBER:
                expr.operands[operand_index].kind = OPERAND_CONSTANT;
                expr.operands[operand_index++].constant.value = tok.string;
                break;
            case TOK_STRING:
                expr.operands[operand_index].kind = OPERAND_CONSTANT;
                expr.operands[operand_index++].constant.value = tok.string;
                break;
            case TOK_OPEN_PARENS:
                // TODO incomplete
                parser->open_parens++;
                break;
            case TOK_CLOSE_PARENS:
                // TODO incomplete
                if (parser->open_parens == 0)
                    SYNTAX_ERROR(tok.loc, "no matching `(`");
                else
                    parser->open_parens--;
                break;
            case TOK_OPEN_SQUARE:
                // TODO incomplete
                parser->open_square++;
                break;
            case TOK_CLOSE_SQUARE:
                // TODO incomplete
                if (parser->open_square == 0)
                    SYNTAX_ERROR(tok.loc, "no matching `[`");
                else
                    parser->open_square--;
                break;
            case TOK_OPEN_CURLY:
                // TODO incomplete
                parser->open_curly++;
                break;
            case TOK_CLOSE_CURLY:
                // TODO incomplete
                if (parser->open_curly == 0)
                    SYNTAX_ERROR(tok.loc, "no matching `{`");
                else
                    parser->open_curly--;
                break;
            default:
                UNIMPLEMENTED("no implementation for token");
        };
        assert(operand_index != EXPRESSION_CAPACITY);
    } while (!parser_is_end_of_expression(parser));

    for (int prec = MAX_PRECEDENCE; prec >= 0; prec--) {
        IntermediateOperationTableEntry* entry = by_prec + prec;
        if (entry->length == 0) continue;
        for (size_t i = 0; i < entry->length; i++) {
            size_t index = ((unsigned)prec == POW_PREC) ? entry->length - 1 - i : i;
            expr.operations[expr.n_operations++] = entry->operations[index];
        }
    }

    return expr;
}

static inline Token
parser_expect_keyword(Parser* parser, Keyword kw)
{
    // TODO: expose some keyword table to lookup a const char* given Keyword enum
    Token tok = token_stream_consume(parser->ts);
    if (tok.type != TOK_KEYWORD || tok.keyword != kw)
        SYNTAX_ERRORF(tok.loc, "expected keyword %i", kw);
    return tok;
}

static inline Token
parser_expect_token_type(Parser* parser, TokenType type)
{
    // TODO: expose some token type table to lookup a const char* given TokenType
    // enum
    Token tok = token_stream_consume(parser->ts);
    if (tok.type != type) SYNTAX_ERRORF(tok.loc, "expected token type %i", type);
    return tok;
}

static Instruction
parse_for_loop_instruction(Parser* parser)
{
    Instruction inst = {.type = INST_FOR_LOOP};
    // consume for token
    Token tok = parser_expect_keyword(parser, KW_FOR);

    tok = parser_expect_token_type(parser, TOK_IDENTIFIER);
    inst.for_loop.it = tok.string;

    tok = parser_expect_keyword(parser, KW_IN);
    inst.for_loop.iterable_expr = parse_expression(parser);

    tok = parser_expect_token_type(parser, TOK_COLON);

    return inst;
}

static Instruction
parse_unknown_instruction(Parser* parser)
{
    Instruction inst = {.type = NULL_INST};
    (void)token_stream_consume(parser->ts);
    return inst;
}

Instruction
parser_next_instruction(Parser* parser)
{
    Instruction inst = {.type = NULL_INST};

    while (token_stream_peek_type(parser->ts) == TOK_NEWLINE)
        (void)token_stream_consume(parser->ts);

    Token first_token = token_stream_peek(parser->ts);
    if (first_token.type == TOK_KEYWORD) {
        switch (first_token.keyword) {
            case KW_FOR:
                return parse_for_loop_instruction(parser);
            default:
                return parse_unknown_instruction(parser);
        }
    }
    else if (first_token.type == TOK_EOF) {
        (void)token_stream_consume(parser->ts);
        inst.type = INST_EOF;
        return inst;
    }

    inst.type = INST_EXPR;
    inst.expr = parse_expression(parser);
    return inst;
}

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

// TODO: portability
static size_t
filename_offset(const char* filepath)
{
    size_t length = strlen(filepath);
    for (size_t i = 0; i < length; i++) {
        size_t index = length - 1 - i;
        if (filepath[index] == '/') return index + 1;
    }
    return 0;
}

void
lexer_tokenize(Lexer* lexer)
{
    Location start = {
        .line = 1,
        .filename = lexer->filepath + filename_offset(lexer->filepath),
    };
    Scanner scanner = {.loc = start, .ts = &lexer->ts, .srcfile = lexer->srcfile};
    do {
        scan_token(&scanner);
    } while (!scanner.finished);
}

// temporary until arena is implemented
Instruction instructions[100];

Instruction*
lexer_parse_instructions(Lexer* lexer)
{
    memset(instructions, 0, sizeof(Instruction) * 100);
    Parser parser = {.ts = &lexer->ts};
    size_t index = 0;
    do {
        instructions[index] = parser_next_instruction(&parser);
    } while (instructions[index++].type != INST_EOF);
    return instructions;
}
