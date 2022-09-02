#include "lexer.h"

#include <assert.h>
#include <errno.h>
#include <string.h>

#include "lexer_helpers.h"

#define SCANNER_BUF_CAPACITY 4096

typedef struct {
    FILE* srcfile;
    Arena* arena;
    TokenQueue* tq;
    Token token;
    Location loc;
    unsigned int open_parens;
    unsigned int open_curly;
    unsigned int open_square;
    bool finished;
    char c;
    size_t buflen;
    char buf[SCANNER_BUF_CAPACITY];
} Scanner;

static inline void
scanner_getc(Scanner* scanner)
{
    assert(scanner->buflen < SCANNER_BUF_CAPACITY && "scanner buffer overflow");
    scanner->c = fgetc(scanner->srcfile);
    scanner->buf[scanner->buflen++] = scanner->c;
    scanner->loc.col++;
    scanner->buf[scanner->buflen] = '\0';
}

static inline void
scanner_ungetc(Scanner* scanner)
{
    assert(scanner->buflen && "ungetc from empty buffer");
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
            scanner->open_parens += 1;
            scanner->token.type = TOK_OPEN_PARENS;
            return true;
        case ')':
            if (scanner->open_parens == 0) SYNTAX_ERROR(scanner->loc, "no matching `(`");
            scanner->open_parens -= 1;
            scanner->token.type = TOK_CLOSE_PARENS;
            return true;
        case '[':
            scanner->open_square += 1;
            scanner->token.type = TOK_OPEN_SQUARE;
            return true;
        case ']':
            if (scanner->open_square == 0) SYNTAX_ERROR(scanner->loc, "no matching `[`");
            scanner->open_square -= 1;
            scanner->token.type = TOK_CLOSE_SQUARE;
            return true;
        case '{':
            scanner->open_curly += 1;
            scanner->token.type = TOK_OPEN_CURLY;
            return true;
        case '}':
            if (scanner->open_curly == 0) SYNTAX_ERROR(scanner->loc, "no matching `{`");
            scanner->open_curly -= 1;
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
        case '-': {
            // not really a single char token
            char next = scanner_peekc(scanner);
            if (next == '>') {
                scanner_getc(scanner);
                scanner->token.type = TOK_ARROW;
                return true;
            }
            break;
        }
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
        scanner->token.kw = kw;
        scanner->token.type = TOK_KEYWORD;
    }
    else {
        scanner->token.value =
            arena_copy(scanner->arena, scanner->buf, scanner->buflen + 1);
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
    scanner->token.value = arena_copy(scanner->arena, scanner->buf, scanner->buflen + 1);
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
    scanner->token.value =
        arena_copy(scanner->arena, scanner->buf + 1, scanner->buflen - 1);
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
    scanner->token.op = op_from_cstr(scanner->buf);
    scanner->token.type = TOK_OPERATOR;
}

static inline bool
should_tokenize_newlines(Scanner* scanner)
{
    return (
        scanner->open_parens == 0 && scanner->open_square == 0 && scanner->open_curly == 0
    );
}

static void
scan_token(Scanner* scanner)
{
    scanner->buflen = 0;
    memset(&scanner->token, 0, sizeof(Token));

    skip_whitespace(scanner);
    if (scanner->finished) goto push_token;

    skip_comments(scanner);
    if (scanner->finished) goto push_token;

    if (scanner->c == '\n' && !should_tokenize_newlines(scanner)) {
        scanner->loc.line++;
        scanner->loc.col = 0;
        scan_token(scanner);
        return;
    }

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

push_token:
    tq_push(scanner->tq, scanner->token);
}

typedef enum {
    CONSUMABLE_RULE_UNSET,
    DISALLOW_CONDITIONAL_EXPRESSION,
    BLOCK_BEGIN,
} ConsumableParserRule;

typedef struct {
    Arena* arena;
    Scanner* scanner;
    TokenQueue* tq;
    Token previous;
    Token token;
    ConsumableParserRule consumable_rule;
    IndentationStack indent_stack;
    LexicalScopeStack scope_stack;
} Parser;

static inline ConsumableParserRule
consume_rule(Parser* parser)
{
    ConsumableParserRule rule = parser->consumable_rule;
    parser->consumable_rule = CONSUMABLE_RULE_UNSET;
    return rule;
}

static inline Token
get_next_token(Parser* parser)
{
    if (!parser->tq->length) scan_token(parser->scanner);
    Token token = tq_consume(parser->tq);
    parser->previous = parser->token;
    parser->token = token;
    return token;
}

static inline Token
peek_next_token(Parser* parser)
{
    if (!parser->tq->length) scan_token(parser->scanner);
    return tq_peek(parser->tq, 0);
}

static inline Token
peek_forward_n_tokens(Parser* parser, size_t n)
{
    // TODO: should consider if we need a special case for trying to peek past EOF
    // token
    size_t required_scans = (n + 1) - parser->tq->length;
    while (required_scans-- > 0) scan_token(parser->scanner);
    return tq_peek(parser->tq, n);
}

static inline void
discard_next_token(Parser* parser)
{
    tq_discard(parser->tq);
}

// TODO: might be worth implementing a lookup table for this
static bool
is_end_of_expression(Parser* parser, ConsumableParserRule rule)
{
    Token token = peek_next_token(parser);
    switch (token.type) {
        case TOK_OPERATOR:
            return (IS_ASSIGNMENT_OP[token.op]);
        case TOK_COLON:
            return true;
        case TOK_NEWLINE:
            return true;
        case TOK_CLOSE_PARENS:
            return true;
        case TOK_CLOSE_SQUARE:
            return true;
        case TOK_CLOSE_CURLY:
            return true;
        case TOK_COMMA:
            return true;
        case TOK_KEYWORD:
            switch (token.kw) {
                case KW_IF:
                    return rule == DISALLOW_CONDITIONAL_EXPRESSION;
                case KW_AND:
                    return false;
                case KW_OR:
                    return false;
                case KW_NOT:
                    return false;
                case KW_IN:
                    return false;
                case KW_IS:
                    return false;
                default:
                    return true;
            }
            break;
        case TOK_EOF:
            return true;
        default:
            break;
    }
    return false;
}

static Expression* parse_expression(Parser* parser);
static inline Token expect_token_type(Parser* parser, TokenType type);
static inline Token expect_keyword(Parser* parser, Keyword kw);

static ItGroup*
parse_iterable_identifiers(Parser* parser)
{
    ItIdentifierVector vec = itid_vector_init(parser->arena);
    ItGroup* it_group = arena_alloc(parser->arena, sizeof(ItGroup));

    while (peek_next_token(parser).type != TOK_KEYWORD) {
        Token token = get_next_token(parser);
        switch (token.type) {
            case TOK_COMMA:
                // maybe ignoring commas is a bad idea but it seems to me they don't
                // actually have meaning here besides being a delimiter though this
                // will allow for some whacky syntax: `for key,,,,,value,,, in
                // d.items():`
                continue;
            case TOK_OPEN_PARENS: {
                ItIdentifier identifier = {
                    .kind = IT_GROUP, .group = parse_iterable_identifiers(parser)};
                itid_vector_append(&vec, identifier);
                break;
            }
            case TOK_CLOSE_PARENS: {
                goto return_it_group;
                break;
            }
            case TOK_IDENTIFIER: {
                ItIdentifier identifier = {.kind = IT_ID, .name = token.value};
                itid_vector_append(&vec, identifier);
                break;
            }
            default:
                SYNTAX_ERROR(token.loc, "unexpected token");
        }
    }
return_it_group:
    if (vec.count == 0) SYNTAX_ERROR(peek_next_token(parser).loc, "no identifiers given");
    it_group->identifiers = itid_vector_finalize(&vec);
    it_group->identifiers_count = vec.count;
    return it_group;
}

static void
parse_comprehension_body(Parser* parser, ComprehensionBody* body)
{
    Token next;

    ItGroupVector its = itgroup_vector_init(parser->arena);
    ExpressionVector iterables = expr_vector_init(parser->arena);

    do {
        // for
        expect_keyword(parser, KW_FOR);
        // it
        itgroup_vector_append(&its, parse_iterable_identifiers(parser));
        // in
        expect_keyword(parser, KW_IN);
        // some iterable
        parser->consumable_rule = DISALLOW_CONDITIONAL_EXPRESSION;
        expr_vector_append(&iterables, parse_expression(parser));
        // potentially another nested for loop
        body->loop_count += 1;
        next = peek_next_token(parser);
    } while (next.type == TOK_KEYWORD && next.kw == KW_FOR);

    body->its = itgroup_vector_finalize(&its);
    body->iterables = expr_vector_finalize(&iterables);

    // maybe has if condition at the eend
    next = peek_next_token(parser);
    if (next.type == TOK_KEYWORD && next.kw == KW_IF) {
        discard_next_token(parser);
        body->if_expr = parse_expression(parser);
    }
}

static Comprehension*
parse_dict_comprehension(Parser* parser, Expression* key_expr, Expression* val_expr)
{
    Comprehension* comp = arena_alloc(parser->arena, sizeof(Comprehension));
    comp->type = ENCLOSURE_DICT;
    comp->mapped.key_expr = key_expr;
    comp->mapped.val_expr = val_expr;
    parse_comprehension_body(parser, &comp->body);
    expect_token_type(parser, TOK_CLOSE_CURLY);
    return comp;
}

static Operand
parse_mapped_enclosure(Parser* parser)
{
    Operand op = {0};

    // empty dict
    if (peek_next_token(parser).type == TOK_CLOSE_CURLY) {
        Enclosure* enclosure = arena_alloc(parser->arena, sizeof(Enclosure));
        discard_next_token(parser);
        enclosure->type = ENCLOSURE_DICT;
        enclosure->expressions_count = 0;
        op.kind = OPERAND_ENCLOSURE_LITERAL;
        op.enclosure = enclosure;
        return op;
    }

    Expression* first_key = parse_expression(parser);
    expect_token_type(parser, TOK_COLON);
    Expression* first_val = parse_expression(parser);
    Token next_token = peek_next_token(parser);

    // might be a comprehension
    if (next_token.type == TOK_KEYWORD) {
        op.kind = OPERAND_COMPREHENSION,
        op.comp = parse_dict_comprehension(parser, first_key, first_val);
        return op;
    }

    // prepare to continue parsing dict literal
    Enclosure* enclosure = arena_alloc(parser->arena, sizeof(Enclosure));
    op.enclosure = enclosure;
    ExpressionVector vec = expr_vector_init(parser->arena);
    expr_vector_append(&vec, first_key);
    expr_vector_append(&vec, first_val);

    if (next_token.type == TOK_CLOSE_CURLY) {
        discard_next_token(parser);
        enclosure->type = ENCLOSURE_DICT;
        enclosure->expressions = expr_vector_finalize(&vec);
        enclosure->expressions_count = vec.count;
        op.kind = OPERAND_ENCLOSURE_LITERAL;
        return op;
    }

    // continue parsing literal
    else {
        while (peek_next_token(parser).type != TOK_CLOSE_CURLY) {
            expect_token_type(parser, TOK_COMMA);
            // beware trailing comma
            if (peek_next_token(parser).type == TOK_CLOSE_CURLY) break;
            // key:value
            expr_vector_append(&vec, parse_expression(parser));
            expect_token_type(parser, TOK_COLON);
            expr_vector_append(&vec, parse_expression(parser));
        }
        discard_next_token(parser);
        enclosure->type = ENCLOSURE_DICT,
        enclosure->expressions = expr_vector_finalize(&vec);
        enclosure->expressions_count = vec.count;
        op.kind = OPERAND_ENCLOSURE_LITERAL;
        return op;
    }
}

static Comprehension*
parse_sequence_comprehension(Parser* parser, Expression* expr, TokenType closing_token)
{
    Comprehension* comp = arena_alloc(parser->arena, sizeof(Comprehension));
    comp->sequence.expr = expr;
    comp->type = (closing_token == TOK_CLOSE_PARENS) ? ENCLOSURE_TUPLE : ENCLOSURE_LIST;
    parse_comprehension_body(parser, &comp->body);
    expect_token_type(parser, closing_token);
    return comp;
}

static Operand
parse_sequence_enclosure(Parser* parser)
{
    Operand op = {0};

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
        discard_next_token(parser);
        op.kind = OPERAND_ENCLOSURE_LITERAL,
        op.enclosure = arena_alloc(parser->arena, sizeof(Enclosure));
        op.enclosure->type = enclosure_type;
        op.enclosure->expressions_count = 0;
        return op;
    };

    Expression* first_expr = parse_expression(parser);
    Token peek = peek_next_token(parser);

    // tuple with no comma resolves to plain expression
    // ex: (1)  == 1
    //     (1,) != 1
    if (peek.type == TOK_CLOSE_PARENS) {
        discard_next_token(parser);
        op.kind = OPERAND_EXPRESSION;
        op.expr = first_expr;
        return op;
    }

    // sequence comprehension
    if (peek.type == TOK_KEYWORD && peek.kw == KW_FOR) {
        Comprehension* comp =
            parse_sequence_comprehension(parser, first_expr, closing_token_type);
        op.kind = OPERAND_COMPREHENSION;
        op.comp = comp;
        return op;
    }

    // prepare to continue parsing enclosure literal
    Enclosure* enclosure = arena_alloc(parser->arena, sizeof(Enclosure));
    enclosure->type = enclosure_type;
    op.enclosure = enclosure;

    // single element list with no comma resolves to list
    if (peek.type == TOK_CLOSE_SQUARE) {
        discard_next_token(parser);
        enclosure->expressions = arena_alloc(parser->arena, sizeof(Expression**));
        enclosure->expressions[0] = first_expr;
        enclosure->expressions_count = 1;
        op.kind = OPERAND_ENCLOSURE_LITERAL;
        return op;
    }

    // otherwise parse the rest of the enclosure literal
    ExpressionVector vec = expr_vector_init(parser->arena);
    expr_vector_append(&vec, first_expr);

    while (peek_next_token(parser).type != closing_token_type) {
        expect_token_type(parser, TOK_COMMA);
        // beware trailing comma
        if (peek_next_token(parser).type == closing_token_type) break;
        expr_vector_append(&vec, parse_expression(parser));
    }

    discard_next_token(parser);
    enclosure->expressions = expr_vector_finalize(&vec);
    enclosure->expressions_count = vec.count;
    op.kind = OPERAND_ENCLOSURE_LITERAL;
    return op;
}

static Operand
parse_arguments(Parser* parser)
{
    Arguments* args = arena_alloc(parser->arena, sizeof(Arguments));
    Operand op = {.kind = OPERAND_ARGUMENTS, .args = args};

    ExpressionVector values = expr_vector_init(parser->arena);
    StringVector kwds = str_vector_init(parser->arena);

    while (true) {
        // end of arguments
        if (peek_next_token(parser).type == TOK_CLOSE_PARENS) {
            discard_next_token(parser);
            break;
        }

        if (values.count > 0) expect_token_type(parser, TOK_COMMA);

        Token next = peek_forward_n_tokens(parser, 1);

        // keyword argument
        if (next.type == TOK_OPERATOR && next.op == OPERATOR_ASSIGNMENT) {
            str_vector_append(&kwds, expect_token_type(parser, TOK_IDENTIFIER).value);
            discard_next_token(parser);  // consume assignment op
            expr_vector_append(&values, parse_expression(parser));
        }
        // positional argument
        else {
            expr_vector_append(&values, parse_expression(parser));
            args->n_positional++;
        }
    }

    args->values = expr_vector_finalize(&values);
    args->values_count = values.count;
    args->kwds = str_vector_finalize(&kwds);

    return op;
}

static Operand
parse_getitem_arguments(Parser* parser)
{
    Slice* slice;
    Operand op = {0};
    Token peek = peek_next_token(parser);
    Token next;
    // []
    if (peek.type == TOK_CLOSE_SQUARE)
        SYNTAX_ERROR(peek.loc, "some argument is required for the getitem operator");
    // [expr1...
    else if (peek.type != TOK_COLON) {
        Expression* first_expr = parse_expression(parser);
        next = get_next_token(parser);
        // [expr1]
        if (next.type == TOK_CLOSE_SQUARE) {
            op.kind = OPERAND_EXPRESSION;
            op.expr = first_expr;
            return op;
        }
        // we now know we're dealing with a slice
        slice = arena_alloc(parser->arena, sizeof(Slice));
        // [expr1:...
        if (next.type == TOK_COLON) {
            slice->start_expr = first_expr;
            goto slice_stop_expr;
        }
        else {
            SYNTAX_ERROR(next.loc, "expected either `:` or `]`");
        }
    }
    // [:...
    else {
        slice = arena_alloc(parser->arena, sizeof(Slice));
        discard_next_token(parser);
        slice->start_expr = NULL;
    }
slice_stop_expr:
    // [...:...
    peek = peek_next_token(parser);
    // [...:]
    if (peek.type == TOK_CLOSE_SQUARE) {
        discard_next_token(parser);
        slice->stop_expr = NULL;
        slice->step_expr = NULL;
        goto return_slice_operand;
    }
    // [...::...
    if (peek.type == TOK_COLON) {
        discard_next_token(parser);
        slice->stop_expr = NULL;
        goto slice_step_expr;
    }
    // [...:expr2...
    slice->stop_expr = parse_expression(parser);
    next = get_next_token(parser);
    // [...:expr2:...
    if (next.type == TOK_COLON) goto slice_step_expr;
    // [...:expr2]
    else if (next.type == TOK_CLOSE_SQUARE) {
        slice->step_expr = NULL;
        goto return_slice_operand;
    }
    else
        SYNTAX_ERROR(next.loc, "expected either `:` or `]`");
slice_step_expr:
    // [...:...:...
    peek = peek_next_token(parser);
    // [...:...:]
    if (peek.type == TOK_CLOSE_SQUARE) {
        discard_next_token(parser);
        slice->step_expr = NULL;
        goto return_slice_operand;
    }
    // [...:...:expr3]
    slice->step_expr = parse_expression(parser);
    expect_token_type(parser, TOK_CLOSE_SQUARE);
return_slice_operand:
    op.slice = slice;
    op.kind = OPERAND_SLICE;
    return op;
}

static inline Token
expect_keyword(Parser* parser, Keyword kw)
{
    Token tok = get_next_token(parser);
    if (tok.type != TOK_KEYWORD || tok.kw != kw)
        SYNTAX_ERRORF(tok.loc, "expected keyword `%s`", kw_to_cstr(kw));
    return tok;
}

static inline Token
expect_token_type(Parser* parser, TokenType type)
{
    Token tok = get_next_token(parser);
    if (tok.type != type)
        SYNTAX_ERRORF(
            tok.loc,
            "expected token type `%s`, got `%s`",
            token_type_to_cstr(type),
            token_type_to_cstr(tok.type)
        );
    return tok;
}

// TODO: might want some kind of lookup table instead of the long switch statements
// int the future
static Expression*
parse_expression(Parser* parser)
{
    ExpressionTable et = et_init(parser->arena);
    ConsumableParserRule rule = consume_rule(parser);
    do {
        Token tok = get_next_token(parser);
        switch (tok.type) {
            case TOK_KEYWORD: {
                switch (tok.kw) {
                    case KW_IF:
                        if (rule == DISALLOW_CONDITIONAL_EXPRESSION)
                            SYNTAX_ERROR(
                                tok.loc, "conditional expression not allowed here"
                            );
                        et_push_operation_type(&et, OPERATOR_CONDITIONAL_IF);

                        Operand condition_operand = {
                            .kind = OPERAND_EXPRESSION, .expr = parse_expression(parser)};
                        et_push_operand(&et, condition_operand);

                        expect_keyword(parser, KW_ELSE);
                        et_push_operation_type(&et, OPERATOR_CONDITIONAL_ELSE);

                        Operand else_operand = {
                            .kind = OPERAND_EXPRESSION, .expr = parse_expression(parser)};
                        et_push_operand(&et, else_operand);
                        break;
                    case KW_AND:
                        et_push_operation_type(&et, OPERATOR_LOGICAL_AND);
                        break;
                    case KW_OR:
                        et_push_operation_type(&et, OPERATOR_LOGICAL_OR);
                        break;
                    case KW_NOT:
                        et_push_operation_type(&et, OPERATOR_LOGICAL_NOT);
                        break;
                    case KW_IN:
                        et_push_operation_type(&et, OPERATOR_IN);
                        break;
                    case KW_IS:
                        et_push_operation_type(&et, OPERATOR_IS);
                        break;
                    default:
                        SYNTAX_ERRORF(
                            tok.loc, "not expecting keyword `%s` here", kw_to_cstr(tok.kw)
                        );
                        break;
                }
            } break;
            case TOK_OPERATOR: {
                if (tok.op == OPERATOR_PLUS &&
                    (et.previous == ET_NONE || et.previous == ET_OPERATION)) {
                    et_push_operation_type(&et, OPERATOR_POSITIVE);
                }
                else if (tok.op == OPERATOR_MINUS && (et.previous == ET_NONE || et.previous == ET_OPERATION)) {
                    et_push_operation_type(&et, OPERATOR_NEGATIVE);
                }
                else
                    et_push_operation_type(&et, tok.op);
                break;
            }
            case TOK_NUMBER: {
                Operand operand = {.kind = OPERAND_TOKEN, .token = tok};
                et_push_operand(&et, operand);
                break;
            }
            case TOK_STRING: {
                Operand operand = {.kind = OPERAND_TOKEN, .token = tok};
                et_push_operand(&et, operand);
                break;
            }
            case TOK_IDENTIFIER: {
                Operand operand = {.kind = OPERAND_TOKEN, .token = tok};
                et_push_operand(&et, operand);
                break;
            }
            case TOK_OPEN_PARENS: {
                if (parser->previous.type == TOK_OPERATOR || et.operands_count == 0)
                    et_push_operand(&et, parse_sequence_enclosure(parser));
                else {
                    et_push_operation_type(&et, OPERATOR_CALL);
                    et_push_operand(&et, parse_arguments(parser));
                }
                break;
            }
            case TOK_OPEN_SQUARE: {
                if (parser->previous.type == TOK_OPERATOR || et.operands_count == 0)
                    et_push_operand(&et, parse_sequence_enclosure(parser));
                else {
                    et_push_operation_type(&et, OPERATOR_GET_ITEM);
                    et_push_operand(&et, parse_getitem_arguments(parser));
                }
                break;
            }
            case TOK_OPEN_CURLY: {
                et_push_operand(&et, parse_mapped_enclosure(parser));
                break;
            }
            case TOK_DOT: {
                et_push_operation_type(&et, OPERATOR_GET_ATTR);
                Operand right = {
                    .kind = OPERAND_TOKEN,
                    .token = expect_token_type(parser, TOK_IDENTIFIER)};
                et_push_operand(&et, right);
                break;
            }
            default:
                fprintf(
                    stderr, "loc: %s:%u:%u\n", tok.loc.filename, tok.loc.line, tok.loc.col
                );
                UNIMPLEMENTED("token not recognized within expression parsing");
        };
    } while (!is_end_of_expression(parser, rule));

    return et_to_expr(&et);
}

Statement parse_statement(Parser* parser);

static void
consume_newline_tokens(Parser* parser)
{
    while (peek_next_token(parser).type == TOK_NEWLINE) discard_next_token(parser);
}

static Block
parse_block(Parser* parser, unsigned int parent_indent)
{
    StatementVector vec = stmt_vector_init(parser->arena);

    parser->consumable_rule = BLOCK_BEGIN;
    Statement first_body_stmt = parse_statement(parser);
    if (first_body_stmt.loc.col <= parent_indent)
        SYNTAX_ERROR(first_body_stmt.loc, "expected indentation");
    stmt_vector_append(&vec, first_body_stmt);

    for (;;) {
        consume_newline_tokens(parser);
        Token peek = peek_next_token(parser);
        if (peek.loc.col < first_body_stmt.loc.col) {
            if (peek.loc.col > parent_indent)
                SYNTAX_ERROR(peek.loc, "inconsistent indentation");
            break;
        }
        if (peek.loc.col > first_body_stmt.loc.col)
            SYNTAX_ERROR(peek.loc, "inconsistent indentation");
        stmt_vector_append(&vec, parse_statement(parser));
    }

    return (Block){.stmts_count = vec.count, .stmts = stmt_vector_finalize(&vec)};
}

static ImportPath
parse_import_path(Parser* parser)
{
    StringVector vec = str_vector_init(parser->arena);
    str_vector_append(&vec, expect_token_type(parser, TOK_IDENTIFIER).value);
    while (peek_next_token(parser).type == TOK_DOT) {
        discard_next_token(parser);
        str_vector_append(&vec, expect_token_type(parser, TOK_IDENTIFIER).value);
    }
    return (ImportPath){
        .dotted_path = str_vector_finalize(&vec), .path_count = vec.count};
}

static void
parse_import_group(Parser* parser, ImportStatement* stmt)
{
    StringVector what_vec = str_vector_init(parser->arena);
    StringVector as_vec = str_vector_init(parser->arena);
    TokenType end_of_stmt;

    if (peek_next_token(parser).type == TOK_OPEN_PARENS) {
        discard_next_token(parser);
        Token peek = peek_next_token(parser);
        if (peek.type == TOK_CLOSE_PARENS)
            SYNTAX_ERROR(peek.loc, "empty import statement");
        end_of_stmt = TOK_CLOSE_PARENS;
    }
    else {
        end_of_stmt = TOK_NEWLINE;
    }

    for (;;) {
        // what ident
        str_vector_append(&what_vec, expect_token_type(parser, TOK_IDENTIFIER).value);
        // maybe kw_as + as ident
        Token peek = peek_next_token(parser);
        if (peek.type == TOK_KEYWORD && peek.kw == KW_AS) {
            discard_next_token(parser);
            str_vector_append(&as_vec, expect_token_type(parser, TOK_IDENTIFIER).value);
            peek = peek_next_token(parser);
        }
        else
            str_vector_append(&as_vec, NULL);

        // maybe comma
        if (peek.type == TOK_COMMA) {
            discard_next_token(parser);
            continue;
        }
        // maybe end of statement
        else if (peek.type == end_of_stmt) {
            discard_next_token(parser);
            break;
        }
        else
            SYNTAX_ERROR(peek.loc, "unexpected token");
    }

    stmt->what = str_vector_finalize(&what_vec);
    stmt->what_count = what_vec.count;
    stmt->as = str_vector_finalize(&as_vec);
}

static TypeInfo
parse_type_hint(Parser* parser)
{
    char* ident_str = expect_token_type(parser, TOK_IDENTIFIER).value;
    PythonType type = cstr_to_python_type(ident_str);
    return (TypeInfo){
        .type = type, .class_name = (type == PYTYPE_OBJECT) ? ident_str : NULL};
}

static ForLoopStatement*
parse_for_loop(Parser* parser, unsigned int indent)
{
    ForLoopStatement* for_loop = arena_alloc(parser->arena, sizeof(ForLoopStatement));
    discard_next_token(parser);
    for_loop->it = parse_iterable_identifiers(parser);
    expect_keyword(parser, KW_IN);
    for_loop->iterable = parse_expression(parser);
    expect_token_type(parser, TOK_COLON);
    for_loop->body = parse_block(parser, indent);
    return for_loop;
}

static WhileStatement*
parse_while_loop(Parser* parser, unsigned int indent)
{
    discard_next_token(parser);
    WhileStatement* while_loop = arena_alloc(parser->arena, sizeof(WhileStatement));
    while_loop->condition = parse_expression(parser);
    expect_token_type(parser, TOK_COLON);
    while_loop->body = parse_block(parser, indent);
    return while_loop;
}

static ImportStatement*
parse_import_statement(Parser* parser)
{
    ImportStatement* import = arena_alloc(parser->arena, sizeof(ImportStatement));
    discard_next_token(parser);
    import->from = parse_import_path(parser);
    Token peek = peek_next_token(parser);
    if (peek.type == TOK_KEYWORD && peek.kw == KW_AS) {
        discard_next_token(parser);
        import->as = arena_alloc(parser->arena, sizeof(char**));
        import->as[0] = expect_token_type(parser, TOK_IDENTIFIER).value;
    }
    return import;
}

static ConditionalStatement*
parse_if_statement(Parser* parser, unsigned int indent)
{
    // parse if condition and body
    discard_next_token(parser);
    ConditionalStatement* conditional =
        arena_alloc(parser->arena, sizeof(ConditionalStatement));
    conditional->condition = parse_expression(parser);
    expect_token_type(parser, TOK_COLON);
    conditional->body = parse_block(parser, indent);

    // parse variable number of elif statements
    // as long as the indentation level hasn't changed
    ElifStatementVector vec = elif_vector_init(parser->arena);
    Token peek;
    for (;;) {
        consume_newline_tokens(parser);
        peek = peek_next_token(parser);
        if (peek.type == TOK_KEYWORD && peek.kw == KW_ELIF && peek.loc.col == indent) {
            discard_next_token(parser);
            ElifStatement elif = {0};
            elif.condition = parse_expression(parser);
            expect_token_type(parser, TOK_COLON);
            elif.body = parse_block(parser, indent);
            elif_vector_append(&vec, elif);
        }
        else {
            break;
        }
    }
    conditional->elifs = elif_vector_finalize(&vec);
    conditional->elifs_count = vec.count;

    // maybe there is an else statment,
    // we have the next token in `peek` already
    if (peek.type == TOK_KEYWORD && peek.kw == KW_ELSE && peek.loc.col == indent) {
        discard_next_token(parser);
        expect_token_type(parser, TOK_COLON);
        conditional->else_body = parse_block(parser, indent);
    }
    return conditional;
}

static TryStatement*
parse_try_statement(Parser* parser, unsigned int indent)
{
    discard_next_token(parser);
    expect_token_type(parser, TOK_COLON);
    TryStatement* try_stmt = arena_alloc(parser->arena, sizeof(TryStatement));
    try_stmt->try_body = parse_block(parser, indent);

    ExceptStatementVector excepts = except_vector_init(parser->arena);
    Token peek;

    for (;;) {
        // start at expect
        consume_newline_tokens(parser);
        Token begin = expect_keyword(parser, KW_EXCEPT);
        if (begin.loc.col != indent) SYNTAX_ERROR(begin.loc, "inconsistent indentation");
        // we will need a str vector for exception names
        StringVector exceptions = str_vector_init(parser->arena);
        // parse either: an identifier
        peek = peek_next_token(parser);
        if (peek.type == TOK_IDENTIFIER) {
            discard_next_token(parser);
            str_vector_append(&exceptions, peek.value);
        }
        // or: open parens, variable number of identifiers, close parens
        else if (peek.type == TOK_OPEN_PARENS) {
            discard_next_token(parser);
            str_vector_append(
                &exceptions, expect_token_type(parser, TOK_IDENTIFIER).value
            );
            peek = peek_next_token(parser);
            while (peek.type != TOK_CLOSE_PARENS) {
                expect_token_type(parser, TOK_COMMA);
                str_vector_append(
                    &exceptions, expect_token_type(parser, TOK_IDENTIFIER).value
                );
                peek = peek_next_token(parser);
            }
            discard_next_token(parser);
        }
        else {
            SYNTAX_ERRORF(
                peek.loc,
                "unexpected token type (%s) (note bare excepts not "
                "allowed)",
                token_type_to_cstr(peek.type)
            );
        }
        // done parsing exception names
        ExceptStatement except = {
            .exceptions = str_vector_finalize(&exceptions),
            .exceptions_count = exceptions.count};
        peek = peek_next_token(parser);
        // maybe parse as
        if (peek.type == TOK_KEYWORD && peek.kw == KW_AS) {
            discard_next_token(parser);
            except.as = expect_token_type(parser, TOK_IDENTIFIER).value;
        }
        else
            except.as = NULL;
        expect_token_type(parser, TOK_COLON);
        // parse body;
        except.body = parse_block(parser, indent);
        // accumulate except statements and consider repeating
        except_vector_append(&excepts, except);
        consume_newline_tokens(parser);
        peek = peek_next_token(parser);
        if (peek.loc.col < indent || peek.type != TOK_KEYWORD || peek.kw != KW_EXCEPT)
            break;
    }
    try_stmt->excepts = except_vector_finalize(&excepts);
    try_stmt->excepts_count = excepts.count;
    if (peek.loc.col < indent) return try_stmt;
    // maybe parse else
    if (peek.type == TOK_KEYWORD && peek.kw == KW_ELSE) {
        discard_next_token(parser);
        expect_token_type(parser, TOK_COLON);
        try_stmt->else_body = parse_block(parser, indent);
    }
    // maybe parse finally
    consume_newline_tokens(parser);
    peek = peek_next_token(parser);
    if (peek.type == TOK_KEYWORD && peek.kw == KW_FINALLY) {
        discard_next_token(parser);
        expect_token_type(parser, TOK_COLON);
        try_stmt->finally_body = parse_block(parser, indent);
    }

    return try_stmt;
}

static WithStatement*
parse_with_statement(Parser* parser, unsigned int indent)
{
    discard_next_token(parser);
    WithStatement* with = arena_alloc(parser->arena, sizeof(WithStatement));
    with->ctx_manager = parse_expression(parser);
    if (peek_next_token(parser).type != TOK_COLON) {
        expect_keyword(parser, KW_AS);
        with->as = expect_token_type(parser, TOK_IDENTIFIER).value;
    }
    expect_token_type(parser, TOK_COLON);
    with->body = parse_block(parser, indent);
    return with;
}

static FunctionStatement*
parse_function_statement(Parser* parser, Location loc)
{
    discard_next_token(parser);
    FunctionStatement* func = arena_alloc(parser->arena, sizeof(FunctionStatement));

    // parse name
    func->name = expect_token_type(parser, TOK_IDENTIFIER).value;
    consume_newline_tokens(parser);

    // begin signature
    expect_token_type(parser, TOK_OPEN_PARENS);
    StringVector params = str_vector_init(parser->arena);
    TypeInfoVector types = typing_vector_init(parser->arena);
    ExpressionVector defaults = expr_vector_init(parser->arena);

    LexicalScope* parent_scope = scope_stack_peek(&parser->scope_stack);
    if (parent_scope->kind == SCOPE_FUNCTION || parent_scope->kind == SCOPE_METHOD)
        SYNTAX_ERROR(loc, "nested functions not supported");
    else if (parent_scope->kind == SCOPE_CLASS) {
        // parse and infer type of `self` param
        Token self_token = get_next_token(parser);
        if (self_token.type != TOK_IDENTIFIER) {
            SYNTAX_ERROR(self_token.loc, "expecting `self` param for method def");
        }
        str_vector_append(&params, self_token.value);
        TypeInfo typing = {.type = PYTYPE_OBJECT, .class_name = parent_scope->cls->name};
        typing_vector_append(&types, typing);
    }

    Token peek = peek_next_token(parser);

    while (peek.type != TOK_CLOSE_PARENS) {
        if (params.count > 0) expect_token_type(parser, TOK_COMMA);
        Token param = expect_token_type(parser, TOK_IDENTIFIER);
        str_vector_append(&params, param.value);
        expect_token_type(parser, TOK_COLON);
        typing_vector_append(&types, parse_type_hint(parser));
        peek = peek_next_token(parser);
        if (peek.type == TOK_OPERATOR && peek.op == OPERATOR_ASSIGNMENT) {
            discard_next_token(parser);
            expr_vector_append(&defaults, parse_expression(parser));
            peek = peek_next_token(parser);
        }
        else if (defaults.count > 0) {
            SYNTAX_ERROR(param.loc, "non default argument follows default argument");
        }
    }
    discard_next_token(parser);  // close parens

    TypeInfo return_type = {.type = PYTYPE_NONE};
    if (peek_next_token(parser).type == TOK_ARROW) {
        discard_next_token(parser);
        return_type = parse_type_hint(parser);
    }

    Signature sig = {
        .return_type = return_type,
        .defaults = expr_vector_finalize(&defaults),
        .defaults_count = defaults.count,
        .params = str_vector_finalize(&params),
        .params_count = params.count,
        .types = typing_vector_finalize(&types)};
    func->sig = sig;

    // init functions lexical scope
    expect_token_type(parser, TOK_COLON);
    LexicalScope* fn_scope = scope_init(parser->arena);
    fn_scope->kind = (parent_scope->kind == SCOPE_CLASS) ? SCOPE_METHOD : SCOPE_FUNCTION;
    fn_scope->func = func;
    for (size_t i = 0; i < sig.params_count; i++) {
        Variable* local_var = arena_alloc(parser->arena, sizeof(Variable));
        local_var->identifier = sig.params[i];
        local_var->type = sig.types[i];
        Symbol local_sym = {.kind = SYM_VARIABLE, .variable = local_var};
        symbol_hm_put(&fn_scope->hm, local_sym);
    }

    // parse body
    scope_stack_push(&parser->scope_stack, fn_scope);
    func->scope = fn_scope;
    func->body = parse_block(parser, loc.col);
    scope_stack_pop(&parser->scope_stack);

    // add function to parents lexical scope
    Symbol sym = {.kind = SYM_FUNCTION, .func = func};
    symbol_hm_put(&parent_scope->hm, sym);
    return func;
}

static ClassStatement*
parse_class_statement(Parser* parser, unsigned int indent)
{
    discard_next_token(parser);
    ClassStatement* cls = arena_alloc(parser->arena, sizeof(ClassStatement));
    cls->name = expect_token_type(parser, TOK_IDENTIFIER).value;
    if (peek_next_token(parser).type == TOK_OPEN_PARENS) {
        discard_next_token(parser);
        cls->base = expect_token_type(parser, TOK_IDENTIFIER).value;
        expect_token_type(parser, TOK_CLOSE_PARENS);
    }
    expect_token_type(parser, TOK_COLON);
    LexicalScope* cls_scope = scope_init(parser->arena);
    cls_scope->kind = SCOPE_CLASS;
    cls_scope->cls = cls;
    scope_stack_push(&parser->scope_stack, cls_scope);
    cls->scope = cls_scope;
    cls->body = parse_block(parser, indent);
    scope_stack_pop(&parser->scope_stack);
    Symbol sym = {.kind = SYM_CLASS, .cls = cls};
    symbol_hm_put(&scope_stack_peek(&parser->scope_stack)->hm, sym);
    return cls;
}

static AssignmentStatement*
parse_assignment_statement(Parser* parser, Expression* assign_to)
{
    AssignmentStatement* assignment =
        arena_alloc(parser->arena, sizeof(AssignmentStatement));
    assignment->storage = assign_to;
    assignment->op_type = expect_token_type(parser, TOK_OPERATOR).op;
    assignment->value = parse_expression(parser);
    if (assignment->op_type == OPERATOR_ASSIGNMENT && assign_to->operations_count == 0) {
        LexicalScope* scope = scope_stack_peek(&parser->scope_stack);
        Variable* var = arena_alloc(parser->arena, sizeof(Variable));
        var->identifier = assign_to->operands[0].token.value;
        var->type = (TypeInfo){.type = PYTYPE_UNTYPED};
        Symbol sym = {.kind = SYM_VARIABLE, .variable = var};
        symbol_hm_put(&scope->hm, sym);
    }
    return assignment;
}

static AnnotationStatement*
parse_annotation_statement(Parser* parser, char* identifier)
{
    AnnotationStatement* annotation =
        arena_alloc(parser->arena, sizeof(AnnotationStatement));
    annotation->identifier = identifier;
    discard_next_token(parser);
    annotation->type = parse_type_hint(parser);

    Token peek = peek_next_token(parser);
    if (peek.type != TOK_NEWLINE) {
        if (peek.type != TOK_OPERATOR && peek.op != OPERATOR_ASSIGNMENT)
            SYNTAX_ERROR(peek.loc, "expecting either `newline` or `=`");
        discard_next_token(parser);
        annotation->initial = parse_expression(parser);
        expect_token_type(parser, TOK_NEWLINE);
    }

    LexicalScope* scope = scope_stack_peek(&parser->scope_stack);
    if (scope->kind == SCOPE_CLASS) {
        // TODO: implement class members
        return annotation;
    }
    else {
        Symbol sym = {
            .kind = SYM_VARIABLE,
            .variable = arena_alloc(parser->arena, sizeof(Variable))};
        sym.variable->identifier = identifier;
        sym.variable->type = annotation->type;
        symbol_hm_put(&scope->hm, sym);
    }
    return annotation;
}

Statement
parse_statement(Parser* parser)
{
    ConsumableParserRule rule = consume_rule(parser);
    Statement stmt = {.kind = NULL_STMT};

    consume_newline_tokens(parser);
    Token peek = peek_next_token(parser);
    stmt.loc = peek.loc;
    indent_check(&parser->indent_stack, stmt.loc, rule == BLOCK_BEGIN);

    if (peek.type == TOK_KEYWORD) {
        switch (peek.kw) {
            case KW_FOR: {
                stmt.kind = STMT_FOR_LOOP;
                stmt.for_loop = parse_for_loop(parser, stmt.loc.col);
                return stmt;
            }
            case KW_PASS:
                discard_next_token(parser);
                expect_token_type(parser, TOK_NEWLINE);
                stmt.kind = STMT_NO_OP;
                return stmt;
            case KW_WHILE: {
                stmt.kind = STMT_WHILE;
                stmt.while_loop = parse_while_loop(parser, stmt.loc.col);
                return stmt;
            }
            case KW_IMPORT: {
                stmt.kind = STMT_IMPORT;
                stmt.import = parse_import_statement(parser);
                return stmt;
            }
            case KW_FROM: {
                discard_next_token(parser);
                stmt.kind = STMT_IMPORT;
                stmt.import = arena_alloc(parser->arena, sizeof(ImportStatement));
                stmt.import->from = parse_import_path(parser);
                expect_keyword(parser, KW_IMPORT);
                parse_import_group(parser, stmt.import);
                return stmt;
            }
            case KW_IF: {
                stmt.kind = STMT_IF;
                stmt.conditional = parse_if_statement(parser, stmt.loc.col);
                return stmt;
            }
            case KW_TRY: {
                stmt.kind = STMT_TRY;
                stmt.try_stmt = parse_try_statement(parser, stmt.loc.col);
                return stmt;
            }
            case KW_WITH: {
                stmt.kind = STMT_WITH;
                stmt.with = parse_with_statement(parser, stmt.loc.col);
                return stmt;
            }
            case KW_DEF: {
                stmt.kind = STMT_FUNCTION;
                stmt.func = parse_function_statement(parser, stmt.loc);
                return stmt;
            }
            case KW_CLASS: {
                stmt.kind = STMT_CLASS;
                stmt.cls = parse_class_statement(parser, stmt.loc.col);
                return stmt;
            }
            default:
                break;
        }
    }
    if (peek.type == TOK_EOF) {
        discard_next_token(parser);
        stmt.kind = STMT_EOF;
        return stmt;
    }

    Expression* expr = parse_expression(parser);

    switch (peek_next_token(parser).type) {
        case TOK_OPERATOR:
            stmt.kind = STMT_ASSIGNMENT;
            stmt.assignment = parse_assignment_statement(parser, expr);
            break;
        case TOK_COLON:
            assert(
                expr->operations_count == 0 &&
                "annotations left expresson should just be an identifier"
            );
            stmt.kind = STMT_ANNOTATION;
            stmt.annotation =
                parse_annotation_statement(parser, expr->operands[0].token.value);
            break;
        default:
            stmt.kind = STMT_EXPR;
            stmt.expr = expr;
    }
    return stmt;
}

#define LEXER_STATEMENTS_CHUNK_SIZE 64

Lexer
lex_file(const char* filepath)
{
    FILE* file = fopen(filepath, "r");
    if (!file) {
        fprintf(stderr, "ERROR: failed to open file -- %s\n", strerror(errno));
        exit(1);
    }
    Arena* arena = arena_init();
    Lexer lexer = {.arena = arena, .top_level = scope_init(arena)};
    Location start_location = {
        .line = 1,
        .filename = filepath + filename_offset(filepath),
    };
    TokenQueue tq = {0};
    Scanner scanner = {.arena = arena, .loc = start_location, .srcfile = file, .tq = &tq};
    Parser parser = {.arena = arena, .scanner = &scanner, .tq = &tq};
    scope_stack_push(&parser.scope_stack, lexer.top_level);

    size_t statements_capacity = LEXER_STATEMENTS_CHUNK_SIZE;
    lexer.statements = malloc(sizeof(Statement) * statements_capacity);
    if (!lexer.statements) out_of_memory();

    do {
        if (lexer.n_statements >= statements_capacity) {
            statements_capacity += LEXER_STATEMENTS_CHUNK_SIZE;
            lexer.statements =
                realloc(lexer.statements, sizeof(Statement) * statements_capacity);
            if (!lexer.statements) out_of_memory();
        }
        lexer.statements[lexer.n_statements] = parse_statement(&parser);
    } while (lexer.statements[lexer.n_statements++].kind != STMT_EOF);

    fclose(file);

    return lexer;
}

void
lexer_free(Lexer* lexer)
{
    free(lexer->statements);
    arena_free(lexer->arena);
}

#if DEBUG
Token*
tokenize_file(const char* filepath, size_t* token_count)
{
    FILE* file = fopen(filepath, "r");
    if (!file) {
        fprintf(stderr, "ERROR: unable to open %s (%s)\n", filepath, strerror(errno));
        exit(1);
    }

    Location start_location = {
        .line = 1,
        .filename = filepath + filename_offset(filepath),
    };
    TokenQueue tq = {0};
    Arena* arena = arena_init();
    Scanner scanner = {.arena = arena, .loc = start_location, .srcfile = file, .tq = &tq};

    size_t count = 0;
    size_t capacity = 64;
    Token* tokens = malloc(sizeof(Token) * capacity);
    if (!tokens) out_of_memory();

    do {
        if (count == capacity) {
            capacity *= 2;
            tokens = realloc(tokens, sizeof(Token) * capacity);
            if (!tokens) out_of_memory();
        }
        scan_token(&scanner);
        tq_consume(&tq);
        tokens[count++] = scanner.token;
    } while (scanner.token.type != TOK_EOF);

    *token_count = count;
    fclose(file);
    return tokens;
}
#endif
