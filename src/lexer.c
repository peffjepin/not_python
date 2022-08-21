#include "lexer.h"

#include <assert.h>
#include <errno.h>
#include <string.h>

#include "lexer_types.h"

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
    Arena* arena;
    Token token;
    Location loc;
    bool finished;
    char c;
    size_t buflen;
    char buf[ARENA_STRING_CHUNK_SIZE];
} Scanner;

static inline void
scanner_getc(Scanner* scanner)
{
    scanner->c = fgetc(scanner->srcfile);
    scanner->buf[scanner->buflen++] = scanner->c;
    scanner->loc.col++;
    scanner->buf[scanner->buflen] = '\0';
}

static inline void
scanner_ungetc(Scanner* scanner)
{
    scanner->loc.col--;
    ungetc(scanner->buf[--scanner->buflen], scanner->srcfile);
}

static inline char
scanner_peekc(Scanner* scanner)
{
    char c = fgetc(scanner->srcfile);
    ungetc(c, scanner->srcfile);
    return c;
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

static inline void
skip_whitespace(Scanner* scanner)
{
    while (scanner->buflen == 0) {
        scanner_getc(scanner);
        switch (scanner->c) {
            case ' ':
                scanner->buflen--;
                break;
            case '\t':
                scanner->buflen--;
                break;
            case EOF:
                scanner->token.type = TOK_EOF;
                scanner->token.loc = scanner->loc;
                scanner->finished = true;
                arena_put_token(scanner->arena, scanner->token);
                return;
            default:
                break;
        }
    }
}

static inline void
skip_comments(Scanner* scanner)
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
            arena_put_token(scanner->arena, scanner->token);
            return;
        }
        // replace '#' with the newline
        scanner->buf[scanner->buflen] = '\n';
        scanner->c = '\n';
    }
}

static inline bool
handle_single_char_tokens(Scanner* scanner)
{
    switch (scanner->c) {
        case '\n':
            scanner->token.type = TOK_NEWLINE;
            scanner->loc.line++;
            scanner->loc.col = 0;
            return true;
        case '(':
            scanner->token.type = TOK_OPEN_PARENS;
            return true;
        case ')':
            scanner->token.type = TOK_CLOSE_PARENS;
            return true;
        case '[':
            scanner->token.type = TOK_OPEN_SQUARE;
            return true;
        case ']':
            scanner->token.type = TOK_CLOSE_SQUARE;
            return true;
        case '{':
            scanner->token.type = TOK_OPEN_CURLY;
            return true;
        case '}':
            scanner->token.type = TOK_CLOSE_CURLY;
            return true;
        case ':':
            scanner->token.type = TOK_COLON;
            return true;
        case ',':
            scanner->token.type = TOK_COMMA;
            return true;
        case '.':
            if (CHAR_IS_ALPHA(scanner_peekc(scanner))) {
                scanner->token.type = TOK_DOT;
                return true;
            }
            return false;
        default:
            return false;
    }
    return false;
}

static inline void
tokenize_word(Scanner* scanner)
{
    for (scanner_getc(scanner); CHAR_IS_ALPHA(scanner->c) || CHAR_IS_NUMERIC(scanner->c);
         scanner_getc(scanner))
        ;
    scanner_ungetc(scanner);
    scanner->buf[scanner->buflen] = '\0';
    Keyword kw;
    if ((kw = is_keyword(scanner->buf))) {
        scanner->token.value_ref = kw;
        scanner->token.type = TOK_KEYWORD;
    }
    else {
        scanner->token.value_ref = arena_put_string(scanner->arena, scanner->buf);
        scanner->token.type = TOK_IDENTIFIER;
    }
}

static inline void
tokenize_numeric(Scanner* scanner)
{
    for (scanner_getc(scanner);
         CHAR_IS_NUMERIC(scanner->c) || scanner->c == '.' || scanner->c == '_';
         scanner_getc(scanner)) {
        if (scanner->c == '_') scanner->buflen--;  // skip `_` such as in `1_000_000`
    }
    scanner_ungetc(scanner);
    scanner->buf[scanner->buflen] = '\0';
    scanner->token.value_ref = arena_put_string(scanner->arena, scanner->buf);
    scanner->token.type = TOK_NUMBER;
}

static inline void
tokenize_string_literal(Scanner* scanner)
{
    char opening_quote = scanner->c;
    for (scanner_getc(scanner);
         scanner->c != opening_quote || scanner->buf[scanner->buflen - 2] == '\\';
         scanner_getc(scanner)) {
        if (scanner->c == EOF)
            SYNTAX_ERROR(scanner->token.loc, "unterminated string literal");
        if (scanner->c == opening_quote) {
            // an escaped version of starting quote
            // we should overwrite the preceding `\` with this quote
            scanner->buf[--scanner->buflen - 1] = scanner->c;
        }
    }
    scanner->buf[scanner->buflen - 1] = '\0';
    scanner->token.value_ref = arena_put_string(scanner->arena, scanner->buf + 1);
    scanner->token.type = TOK_STRING;
}

static inline void
tokenize_operator(Scanner* scanner)
{
    assert(CHAR_IS_OPERATOR(scanner->c));
    for (scanner_getc(scanner); CHAR_IS_OPERATOR(scanner->c); scanner_getc(scanner))
        ;
    scanner_ungetc(scanner);
    scanner->buf[scanner->buflen] = '\0';
    scanner->token.value_ref = op_from_cstr(scanner->buf);
    scanner->token.type = TOK_OPERATOR;
}

static void
scan_token(Scanner* scanner)
{
    scanner->buflen = 0;
    memset(&scanner->token, 0, sizeof(Token));

    skip_whitespace(scanner);
    if (scanner->finished) return;

    skip_comments(scanner);
    if (scanner->finished) return;

    // we're now at the start of a new token
    scanner->token.loc = scanner->loc;

    if (handle_single_char_tokens(scanner))
        ;
    else if (CHAR_IS_ALPHA(scanner->c))
        tokenize_word(scanner);
    else if (CHAR_IS_NUMERIC(scanner->c) || scanner->c == '.')
        tokenize_numeric(scanner);
    else if (scanner->c == '\'' || scanner->c == '"')
        tokenize_string_literal(scanner);
    else
        tokenize_operator(scanner);

    arena_put_token(scanner->arena, scanner->token);
}

typedef struct {
    Arena* arena;
    Token previous;
    Token token;
    size_t current_token_index;
    unsigned int open_parens;
    unsigned int open_square;
    unsigned int open_curly;
    bool inside_getitem;
} Parser;

#define IS_WITHIN_ENCLOSURE(parser)                                                      \
    (parser->open_parens != 0 || parser->open_curly != 0 || parser->open_square != 0)

static inline Token
get_next_token(Parser* parser)
{
    Token token = arena_get_token(parser->arena, parser->current_token_index++);
    parser->previous = parser->token;
    parser->token = token;
    switch (token.type) {
        case NULL_TOKEN:
            UNIMPLEMENTED("waiting on tokenization");
            break;
        case TOK_OPEN_PARENS:
            parser->open_parens++;
            break;
        case TOK_CLOSE_PARENS:
            if (parser->open_parens == 0)
                SYNTAX_ERROR(token.loc, "no matching `(`");
            else
                parser->open_parens--;
            break;
        case TOK_OPEN_SQUARE:
            parser->open_square++;
            break;
        case TOK_CLOSE_SQUARE:
            if (parser->open_square == 0)
                SYNTAX_ERROR(token.loc, "no matching `[`");
            else
                parser->open_square--;
            break;
        case TOK_OPEN_CURLY:
            parser->open_curly++;
            break;
        case TOK_CLOSE_CURLY:
            if (parser->open_curly == 0)
                SYNTAX_ERROR(token.loc, "no matching `{`");
            else
                parser->open_curly--;
            break;
        case TOK_NEWLINE:
            if (IS_WITHIN_ENCLOSURE(parser)) {
                // newlines are ignored within enclosures
                return get_next_token(parser);
            }
            break;
        default:
            break;
    }
    return token;
}

static inline Token
peek_next_token(Parser* parser)
{
    ArenaRef index = parser->current_token_index;
    Token token = arena_get_token(parser->arena, parser->current_token_index);
    if (token.type == NULL_TOKEN) UNIMPLEMENTED("waiting on tokenization");
    if (IS_WITHIN_ENCLOSURE(parser)) {
        while (token.type == TOK_NEWLINE) token = arena_get_token(parser->arena, index++);
    }
    return token;
}

static bool
is_end_of_expression(Parser* parser)
{
    Token token = peek_next_token(parser);
    switch (token.type) {
        case TOK_OPERATOR:
            return (IS_ASSIGNMENT_OP[token.value_ref]);
        case TOK_COLON:
            return (!parser->inside_getitem);
        case TOK_NEWLINE:
            return true;
        case TOK_OPEN_PARENS:
            return true;
        case TOK_CLOSE_PARENS:
            return true;
        case TOK_OPEN_SQUARE:
            return true;
        case TOK_CLOSE_SQUARE:
            return true;
        case TOK_OPEN_CURLY:
            return true;
        case TOK_CLOSE_CURLY:
            return true;
        case TOK_COMMA:
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

typedef struct {
    Expression* buf;
    size_t capacity;
    size_t length;
} ExpressionVector;

static void
expression_vector_append(ExpressionVector* vec, Expression expr)
{
    if (vec->length == vec->capacity) {
        if (vec->capacity == 0)
            vec->capacity = 8;
        else
            vec->capacity *= 2;
        vec->buf = realloc(vec->buf, sizeof(Expression) * vec->capacity);
        if (!vec->buf) {
            fprintf(stderr, "ERROR: out of memory");
            exit(1);
        }
    }
    vec->buf[vec->length++] = expr;
}

static void
expression_vector_free(ExpressionVector* vec)
{
    free(vec->buf);
}

static Expression parse_expression(Parser* parser);
static inline Token expect_token_type(Parser* parser, TokenType type);
static inline Token expect_keyword(Parser* parser, Keyword kw);

static size_t
parse_comprehension_loops(
    Parser* parser, Location loc, ArenaRef* its, ArenaRef* iterables
)
{
    Token next;
    size_t nesting = 0;
    do {
        if (nesting == MAX_COMPREHENSION_NESTING) {
            SYNTAX_ERRORF(
                loc,
                "maximum comprehension nesting (%u) exceeded",
                MAX_COMPREHENSION_NESTING
            );
        }
        // for
        expect_keyword(parser, KW_FOR);
        // it
        Expression it = parse_expression(parser);
        its[nesting] = arena_put_expression(parser->arena, it);
        // in
        expect_keyword(parser, KW_IN);
        // some iterable
        Expression iterable = parse_expression(parser);
        iterables[nesting++] = arena_put_expression(parser->arena, iterable);
        // potentially another nested for loop
        next = peek_next_token(parser);
    } while (next.type == TOK_KEYWORD && next.value_ref == KW_FOR);
    return nesting;
}

static Comprehension
parse_dict_comprehension(
    Parser* parser, Location loc, Expression first_key, Expression first_val
)
{
    MappedComprehension mapped_comp = {
        .key_expr = arena_put_expression(parser->arena, first_key),
        .val_expr = arena_put_expression(parser->arena, first_val),
    };
    ComprehensionBody body = {0};
    body.nesting = parse_comprehension_loops(parser, loc, body.its, body.iterables);

    expect_token_type(parser, TOK_CLOSE_CURLY);
    Comprehension comp = {.type = ENCLOSURE_DICT, .body = body, .mapped = mapped_comp};
    return comp;
}

static Operand
parse_mapped_enclosure(Parser* parser)
{
    Location loc = parser->token.loc;

    // empty dict
    if (peek_next_token(parser).type == TOK_CLOSE_CURLY) {
        get_next_token(parser);
        Enclosure enclosure = {
            .type = ENCLOSURE_DICT,
            .first_expr = 1,
            .last_expr = 0,
        };
        Operand op = {
            .kind = OPERAND_ENCLOSURE_LITERAL,
            .ref = arena_put_enclosure(parser->arena, enclosure)};
        return op;
    }

    Expression first_key = parse_expression(parser);
    expect_token_type(parser, TOK_COLON);
    Expression first_val = parse_expression(parser);
    Token next_token = peek_next_token(parser);

    if (next_token.type == TOK_CLOSE_CURLY) {
        get_next_token(parser);  // consume token
        Enclosure enclosure = {
            .type = ENCLOSURE_DICT,
            .first_expr = arena_put_expression(parser->arena, first_key),
            .last_expr = arena_put_expression(parser->arena, first_val)};
        Operand op = {
            .kind = OPERAND_ENCLOSURE_LITERAL,
            .ref = arena_put_enclosure(parser->arena, enclosure)};
        return op;
    }

    else if (next_token.type == TOK_KEYWORD) {
        Comprehension comp = parse_dict_comprehension(parser, loc, first_key, first_val);
        Operand op = {
            .kind = OPERAND_COMPREHENSION,
            .ref = arena_put_comprehension(parser->arena, comp)};
        return op;
    }

    // continue parsing literal
    else {
        // enclosures are marked by their starting and ending expressions
        // so we wont add any parsed expressions into the arena until we
        // have parsed the entire dict literal
        ExpressionVector vec = {0};
        expression_vector_append(&vec, first_key);
        expression_vector_append(&vec, first_val);
        while (peek_next_token(parser).type != TOK_CLOSE_CURLY) {
            expect_token_type(parser, TOK_COMMA);
            // beware trailing comma
            if (peek_next_token(parser).type == TOK_CLOSE_CURLY) break;
            // key
            expression_vector_append(&vec, parse_expression(parser));
            // :
            expect_token_type(parser, TOK_COLON);
            // value
            expression_vector_append(&vec, parse_expression(parser));
        }
        expect_token_type(parser, TOK_CLOSE_CURLY);
        Enclosure enclosure = {
            .type = ENCLOSURE_DICT,
            .first_expr = arena_put_expression(parser->arena, vec.buf[0])};
        ArenaRef ref;
        for (size_t i = 1; i < vec.length; i++)
            ref = arena_put_expression(parser->arena, vec.buf[i]);
        enclosure.last_expr = ref;
        Operand op = {
            .kind = OPERAND_ENCLOSURE_LITERAL,
            .ref = arena_put_enclosure(parser->arena, enclosure)};
        expression_vector_free(&vec);
        return op;
    }
}

static Comprehension
parse_sequence_comprehension(
    Parser* parser, Location loc, Expression expr, TokenType closing_token
)
{
    SequenceComprehension seq_comp = {
        .expr = arena_put_expression(parser->arena, expr),
    };
    ComprehensionBody body = {0};
    body.nesting = parse_comprehension_loops(parser, loc, body.its, body.iterables);

    expect_token_type(parser, closing_token);
    EnclosureType type =
        (closing_token == TOK_CLOSE_PARENS) ? ENCLOSURE_TUPLE : ENCLOSURE_LIST;
    Comprehension comp = {.type = type, .body = body, .sequence = seq_comp};
    return comp;
}

static Operand
parse_sequence_enclosure(Parser* parser)
{
    Token opening_token = parser->token;
    TokenType closing_token_type;
    EnclosureType enclosure_type;
    if (opening_token.type == TOK_OPEN_PARENS) {
        closing_token_type = TOK_CLOSE_PARENS;
        enclosure_type = ENCLOSURE_TUPLE;
    }
    else {
        closing_token_type = TOK_CLOSE_SQUARE;
        enclosure_type = ENCLOSURE_LIST;
    }

    // empty sequence
    if (peek_next_token(parser).type == closing_token_type) {
        get_next_token(parser);
        Enclosure enclosure = {
            .type = enclosure_type,
            .first_expr = 1,
            .last_expr = 0,
        };
        Operand op = {
            .kind = OPERAND_ENCLOSURE_LITERAL,
            .ref = arena_put_enclosure(parser->arena, enclosure),
        };
        return op;
    }

    Expression first_expr = parse_expression(parser);
    Token peek = peek_next_token(parser);

    if (peek.type == TOK_CLOSE_PARENS) {
        // tuple with no comma resolves to plain expression
        // ex: (1)  == 1
        //     (1,) != 1
        expect_token_type(parser, TOK_CLOSE_PARENS);
        Operand op = {
            .kind = OPERAND_EXPRESSION,
            .ref = arena_put_expression(parser->arena, first_expr)};
        return op;
    }

    else if (peek.type == TOK_CLOSE_SQUARE) {
        // single element list with no comma resolves to list
        expect_token_type(parser, TOK_CLOSE_SQUARE);
        Enclosure enclosure = {
            .type = ENCLOSURE_LIST,
            .first_expr = arena_put_expression(parser->arena, first_expr),
        };
        enclosure.last_expr = enclosure.first_expr;
        Operand op = {
            .kind = OPERAND_ENCLOSURE_LITERAL,
            .ref = arena_put_enclosure(parser->arena, enclosure),
        };
        return op;
    }

    else if (peek.type == TOK_KEYWORD && peek.value_ref == KW_FOR) {
        Comprehension comp = parse_sequence_comprehension(
            parser, opening_token.loc, first_expr, closing_token_type
        );
        Operand op = {
            .kind = OPERAND_COMPREHENSION,
            .ref = arena_put_comprehension(parser->arena, comp)};
        return op;
    }

    // otherwise parse the rest of the enclosure literal
    // accrue expressions in an intermediate vector so that the
    // enclosure.first_expr and enclosure.last_expr ArenaRefs are contiguous
    ExpressionVector vec = {0};
    expression_vector_append(&vec, first_expr);

    while (peek_next_token(parser).type != closing_token_type) {
        expect_token_type(parser, TOK_COMMA);
        // beware trailing comma
        if (peek_next_token(parser).type == closing_token_type) break;
        expression_vector_append(&vec, parse_expression(parser));
    }

    expect_token_type(parser, closing_token_type);
    Enclosure enclosure = {
        .type = enclosure_type,
        .first_expr = arena_put_expression(parser->arena, vec.buf[0])};
    ArenaRef ref = enclosure.first_expr;
    for (size_t i = 1; i < vec.length; i++)
        ref = arena_put_expression(parser->arena, vec.buf[i]);
    enclosure.last_expr = ref;
    Operand op = {
        .kind = OPERAND_ENCLOSURE_LITERAL,
        .ref = arena_put_enclosure(parser->arena, enclosure)};
    expression_vector_free(&vec);
    return op;
}

static Operand
parse_arguments(Parser* parser)
{
    Token begin_args = parser->token;
    Arguments args = {0};
    size_t n_kwds = 0;
    while (true) {
        Token token = get_next_token(parser);
        if (token.type == TOK_CLOSE_PARENS)
            break;
        else if (args.length == MAX_ARGUMENTS) {
            SYNTAX_ERRORF(
                begin_args.loc, "maximum arguments (%u) exceeded", MAX_ARGUMENTS
            );
        }
        Token next = peek_next_token(parser);
        // keyword argument
        if (next.type == TOK_OPERATOR && next.value_ref == OPERATOR_ASSIGNMENT) {
            args.kwds[n_kwds++] = token;
            get_next_token(parser);  // consume assignment op
            args.value_refs[args.length++] =
                arena_put_expression(parser->arena, parse_expression(parser));
        }
        // positional argument
        else {
            args.value_refs[args.length++] =
                arena_put_expression(parser->arena, parse_expression(parser));
            args.n_positional++;
        }
    }
    Operand op = {
        .kind = OPERAND_ARGUMENTS, .ref = arena_put_arguments(parser->arena, args)};
    return op;
}

static Operand
parse_getitem_arguments(Parser* parser)
{
    UNIMPLEMENTED("getitem args parsing is not implemented");
}

static inline Token
expect_keyword(Parser* parser, Keyword kw)
{
    Token tok = get_next_token(parser);
    if (tok.type != TOK_KEYWORD || tok.value_ref != kw)
        SYNTAX_ERRORF(tok.loc, "expected keyword `%s`", kw_to_cstr(kw));
    return tok;
}

static inline Token
expect_token_type(Parser* parser, TokenType type)
{
    Token tok = get_next_token(parser);
    if (tok.type != type)
        SYNTAX_ERRORF(tok.loc, "expected token type `%s`", token_type_to_cstr(type));
    return tok;
}

#define INTERMEDIATE_TABLE_MAX EXPRESSION_MAX_OPERATIONS

// TODO: some smarter data structure for sorting operator precedence
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
        Token tok = get_next_token(parser);
        if (tok.type == NULL_TOKEN) UNIMPLEMENTED("waiting on tokenization");
        switch (tok.type) {
            case TOK_OPERATOR: {
                // FIXME: the right side operand is yet to be parsed so we
                // need to add some assertation that it will be parsed
                unsigned int prec = PRECENDENCE_TABLE[tok.value_ref];
                Operation* operation = &by_prec[prec].operations[by_prec[prec].length++];
                operation->left = operand_index - 1;
                operation->right = operand_index;
                operation->op_type = tok.value_ref;
                break;
            }
            case TOK_NUMBER:
                expr.operands[operand_index].kind = OPERAND_TOKEN;
                expr.operands[operand_index++].token = tok;
                break;
            case TOK_STRING:
                expr.operands[operand_index].kind = OPERAND_TOKEN;
                expr.operands[operand_index++].token = tok;
                break;
            case TOK_IDENTIFIER:
                expr.operands[operand_index].kind = OPERAND_TOKEN;
                expr.operands[operand_index++].token = tok;
                break;
            case TOK_OPEN_PARENS:
                if (parser->previous.type == TOK_OPERATOR || operand_index == 0)
                    expr.operands[operand_index++] = parse_sequence_enclosure(parser);
                else {
                    unsigned int prec = PRECENDENCE_TABLE[OPERATOR_CALL];
                    Operation* operation =
                        &by_prec[prec].operations[by_prec[prec].length++];
                    operation->left = operand_index - 1;
                    operation->right = operand_index;
                    operation->op_type = OPERATOR_CALL;
                    expr.operands[operand_index++] = parse_arguments(parser);
                }
                break;
            case TOK_OPEN_SQUARE:
                if (parser->previous.type == TOK_OPERATOR || operand_index == 0)
                    expr.operands[operand_index++] = parse_sequence_enclosure(parser);
                else {
                    parser->inside_getitem = true;
                    unsigned int prec = PRECENDENCE_TABLE[OPERATOR_GET_ITEM];
                    Operation* operation =
                        &by_prec[prec].operations[by_prec[prec].length++];
                    operation->left = operand_index - 1;
                    operation->right = operand_index;
                    operation->op_type = OPERATOR_GET_ITEM;
                    expr.operands[operand_index++] = parse_getitem_arguments(parser);
                    parser->inside_getitem = false;
                }
                break;
            case TOK_OPEN_CURLY:
                expr.operands[operand_index++] = parse_mapped_enclosure(parser);
                break;
            case TOK_DOT: {
                unsigned int prec = PRECENDENCE_TABLE[OPERATOR_GET_ATTR];
                Operation* operation = &by_prec[prec].operations[by_prec[prec].length++];
                operation->left = operand_index - 1;
                operation->right = operand_index;
                operation->op_type = OPERATOR_GET_ITEM;
                expr.operands[operand_index].kind = OPERAND_TOKEN;
                expr.operands[operand_index++].token =
                    expect_token_type(parser, TOK_IDENTIFIER);
                break;
            }
            default:
                fprintf(
                    stderr, "loc: %s:%u:%u\n", tok.loc.filename, tok.loc.line, tok.loc.col
                );
                UNIMPLEMENTED("no implementation for token");
        };
        assert(operand_index != EXPRESSION_MAX_OPERATIONS + 1);
    } while (!is_end_of_expression(parser));

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

static Statement
parse_for_loop_instruction(Parser* parser)
{
    Statement stmt = {.kind = STMT_FOR_LOOP};
    // consume for token
    Token tok = expect_keyword(parser, KW_FOR);

    tok = expect_token_type(parser, TOK_IDENTIFIER);
    stmt.for_loop.it_ref = tok.value_ref;

    tok = expect_keyword(parser, KW_IN);
    stmt.for_loop.expr_ref =
        arena_put_expression(parser->arena, parse_expression(parser));

    tok = expect_token_type(parser, TOK_COLON);

    return stmt;
}

static Statement
parse_unknown_instruction(Parser* parser)
{
    Statement stmt = {.kind = NULL_STMT};
    parser->current_token_index++;
    return stmt;
}

Statement
parse_statement(Parser* parser)
{
    Statement stmt = {.kind = NULL_STMT};

    while (peek_next_token(parser).type == TOK_NEWLINE) parser->current_token_index++;

    Token first_token = peek_next_token(parser);
    if (first_token.type == TOK_KEYWORD) {
        switch (first_token.value_ref) {
            case KW_FOR:
                return parse_for_loop_instruction(parser);
            default:
                return parse_unknown_instruction(parser);
        }
    }
    else if (first_token.type == TOK_EOF) {
        parser->current_token_index++;
        stmt.kind = STMT_EOF;
        return stmt;
    }

    stmt.kind = STMT_EXPR;
    Expression expr = parse_expression(parser);
    stmt.expr_ref = arena_put_expression(parser->arena, expr);
    return stmt;
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
    Scanner scanner = {.loc = start, .arena = &lexer->arena, .srcfile = lexer->srcfile};
    do {
        scan_token(&scanner);
    } while (!scanner.finished);
}

void
lexer_parse_instructions(Lexer* lexer)
{
    Parser parser = {.arena = &lexer->arena};
    Statement stmt;
    do {
        stmt = parse_statement(&parser);
        arena_put_statement(parser.arena, stmt);
    } while (stmt.kind != STMT_EOF);
}
