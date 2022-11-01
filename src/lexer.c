#include "lexer.h"

#include <assert.h>
#include <errno.h>
#include <string.h>

#include "lexer_helpers.h"
#include "syntax.h"

#define SCANNER_BUF_CAPACITY 4096
#define ASSERT_BUF_CAPACITY 1024

typedef struct {
    FILE* srcfile;
    Arena* arena;
    TokenQueue* tq;
    Token token;
    Location loc;
    FileIndex* index;
    size_t file_offset;
    unsigned int open_parens;
    unsigned int open_curly;
    unsigned int open_square;
    bool finished;
    bool scanning_assert;
    char c;
    size_t buflen;
    char buf[SCANNER_BUF_CAPACITY];
    size_t assert_buflen;
    char assert_buf[ASSERT_BUF_CAPACITY];
} Scanner;

static inline void
scanner_getc(Scanner* scanner)
{
    if (scanner->buflen == SCANNER_BUF_CAPACITY) error("scanner buffer overflow");
    scanner->c = fgetc(scanner->srcfile);
    if (scanner->scanning_assert) {
        if (scanner->assert_buflen == ASSERT_BUF_CAPACITY - 2)
            error("assert buffer overflow");
        switch (scanner->c) {
            case '\n':
                scanner->assert_buf[scanner->assert_buflen++] = '\\';
                scanner->assert_buf[scanner->assert_buflen++] = 'n';
                break;
            case '\t':
                scanner->assert_buf[scanner->assert_buflen++] = '\\';
                scanner->assert_buf[scanner->assert_buflen++] = 't';
                break;
            case '\r':
                scanner->assert_buf[scanner->assert_buflen++] = '\\';
                scanner->assert_buf[scanner->assert_buflen++] = 'r';
                break;
            default:
                scanner->assert_buf[scanner->assert_buflen++] = scanner->c;
        }
    }
    scanner->file_offset++;
    scanner->buf[scanner->buflen++] = scanner->c;
    scanner->loc.col++;
    scanner->buf[scanner->buflen] = '\0';
}

static inline void
scanner_ungetc(Scanner* scanner)
{
    assert(scanner->buflen && "ungetc from empty buffer");
    scanner->file_offset--;
    scanner->loc.col--;
    ungetc(scanner->buf[--scanner->buflen], scanner->srcfile);
    if (scanner->scanning_assert) {
        switch (scanner->c) {
            case '\n':
                scanner->assert_buflen -= 2;
                break;
            case '\t':
                scanner->assert_buflen -= 2;
                break;
            case '\r':
                scanner->assert_buflen -= 2;
                break;
            default:
                scanner->assert_buflen -= 1;
        }
    }
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
                scanner->token.loc = arena_alloc(scanner->arena, sizeof(Location));
                memcpy(scanner->token.loc, &scanner->loc, sizeof(Location));
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
            scanner->token.loc = arena_alloc(scanner->arena, sizeof(Location));
            memcpy(scanner->token.loc, &scanner->loc, sizeof(Location));
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
        case '@':
            scanner->token.type = TOK_DECORATOR;
            return true;
        case '\n':
            file_index_index_line(scanner->index, scanner->file_offset);
            scanner->token.type = TOK_NEWLINE;
            scanner->loc.line++;
            scanner->loc.col = 0;
            return true;
        case '(':
            scanner->open_parens += 1;
            scanner->token.type = TOK_OPEN_PARENS;
            return true;
        case ')':
            if (scanner->open_parens == 0)
                syntax_error(*scanner->index, scanner->loc, 2, "no matching `(`");
            scanner->open_parens -= 1;
            scanner->token.type = TOK_CLOSE_PARENS;
            return true;
        case '[':
            scanner->open_square += 1;
            scanner->token.type = TOK_OPEN_SQUARE;
            return true;
        case ']':
            if (scanner->open_square == 0)
                syntax_error(*scanner->index, scanner->loc, 2, "no matching `[`");
            scanner->open_square -= 1;
            scanner->token.type = TOK_CLOSE_SQUARE;
            return true;
        case '{':
            scanner->open_curly += 1;
            scanner->token.type = TOK_OPEN_CURLY;
            return true;
        case '}':
            if (scanner->open_curly == 0)
                syntax_error(*scanner->index, scanner->loc, 2, "no matching `{`");
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
        scanner->token.value.data =
            arena_copy(scanner->arena, scanner->buf, scanner->buflen + 1);
        scanner->token.value.length = scanner->buflen;
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
    scanner->token.value.data =
        arena_copy(scanner->arena, scanner->buf, scanner->buflen + 1);
    scanner->token.value.length = scanner->buflen;
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
            syntax_error(
                *scanner->index, *scanner->token.loc, 0, "unterminated string literal"
            );
        if (scanner->c == opening_quote) {
            // an escaped version of starting quote
            // we should overwrite the preceding `\` with this quote
            scanner->buf[--scanner->buflen - 1] = scanner->c;
        }
    }
    scanner->buf[scanner->buflen - 1] = '\0';
    scanner->token.value.data =
        arena_copy(scanner->arena, scanner->buf + 1, scanner->buflen - 1);
    scanner->token.value.length = scanner->buflen - 2;
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
        file_index_index_line(scanner->index, scanner->file_offset);
        scanner->loc.line++;
        scanner->loc.col = 0;
        scan_token(scanner);
        return;
    }

    // we're now at the start of a new token
    scanner->token.loc = arena_alloc(scanner->arena, sizeof(Location));
    memcpy(scanner->token.loc, &scanner->loc, sizeof(Location));

    if (handle_single_char_tokens(scanner))
        ;
    else if (CHAR_IS_ALPHA(scanner->c))
        tokenize_word(scanner);
    else if (CHAR_IS_NUMERIC(scanner->c) || scanner->c == '.')
        tokenize_numeric(scanner);
    else if (scanner->c == '\'' || scanner->c == '"')
        tokenize_string_literal(scanner);
    else {
        if (!CHAR_IS_OPERATOR(scanner->c)) {
            syntax_error(*scanner->index, *scanner->token.loc, 0, "unexpected token");
        }
        tokenize_operator(scanner);
    }

push_token:
    if (scanner->token.type == TOK_KEYWORD && scanner->token.kw == KW_ASSERT) {
        memset(scanner->assert_buf, ' ', scanner->token.loc->col - 1);
        memcpy(scanner->assert_buf + scanner->token.loc->col - 1, "assert", 6);
        scanner->assert_buflen += scanner->token.loc->col + 5;
        scanner->scanning_assert = true;
    }
    tq_push(scanner->tq, scanner->token);
}

static SourceString
get_assert_source_code(Scanner* scanner)
{
    while (memcmp(scanner->assert_buf + scanner->assert_buflen - 2, "\\n", 2) == 0)
        // knock off any trailing newlines
        scanner->assert_buflen -= 2;

    scanner->assert_buf[scanner->assert_buflen] = '\0';
    const char* source_code =
        arena_copy(scanner->arena, scanner->assert_buf, scanner->assert_buflen + 1);
    SourceString rtval = {.data = source_code, .length = scanner->assert_buflen};
    scanner->assert_buflen = 0;
    scanner->scanning_assert = false;
    return rtval;
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

    const char* file_namespace;
    size_t file_namespace_length;

    AnnotationStatement* current_class_members;
    size_t current_class_members_capacity;
    size_t current_class_members_count;
    size_t current_class_members_defaults_count;
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
                case KW_TRUE:
                    return false;
                case KW_FALSE:
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
    LexicalScope* scope = scope_stack_peek(&parser->scope_stack);

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
                Variable* var = arena_alloc(parser->arena, sizeof(Variable));
                var->kind = VAR_SEMI_SCOPED;
                var->identifier = token.value;
                Symbol sym = {
                    .kind = SYM_VARIABLE, .variable = var, .identifier = token.value};
                symbol_hm_put(&scope->hm, sym);
                ItIdentifier identifier = {.kind = IT_ID, .name = token.value};
                itid_vector_append(&vec, identifier);
                break;
            }
            default:
                syntax_error(*parser->scanner->index, *token.loc, 0, "unexpected token");
        }
    }
return_it_group:
    if (vec.count == 0)
        syntax_error(
            *parser->scanner->index,
            *peek_next_token(parser).loc,
            0,
            "no identifiers given"
        );
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
    SourceStringVector kwds = str_vector_init(parser->arena);

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
    Slice* slice = NULL;
    Operand op = {0};
    Token peek = peek_next_token(parser);
    Token next;
    // []
    if (peek.type == TOK_CLOSE_SQUARE)
        syntax_error(
            *parser->scanner->index,
            *peek.loc,
            0,
            "some argument is required for the getitem operator"
        );
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
            syntax_error(
                *parser->scanner->index, *next.loc, 0, "expected either `:` or `]`"
            );
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
        syntax_error(*parser->scanner->index, *next.loc, 0, "expected either `:` or `]`");
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
        syntax_errorf(
            *parser->scanner->index, *tok.loc, 0, "expected keyword `%s`", kw_to_cstr(kw)
        );
    return tok;
}

static inline Token
expect_token_type(Parser* parser, TokenType type)
{
    Token tok = get_next_token(parser);
    if (tok.type != type)
        syntax_errorf(
            *parser->scanner->index,
            *tok.loc,
            0,
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
                            syntax_error(
                                *parser->scanner->index,
                                *tok.loc,
                                0,
                                "conditional expression not allowed here"
                            );
                        et_push_operation_type(&et, OPERATOR_CONDITIONAL_IF, tok.loc);

                        Operand condition_operand = {
                            .kind = OPERAND_EXPRESSION, .expr = parse_expression(parser)};
                        et_push_operand(&et, condition_operand);

                        Token else_token = expect_keyword(parser, KW_ELSE);
                        et_push_operation_type(
                            &et, OPERATOR_CONDITIONAL_ELSE, else_token.loc
                        );

                        Operand else_operand = {
                            .kind = OPERAND_EXPRESSION, .expr = parse_expression(parser)};
                        et_push_operand(&et, else_operand);
                        break;
                    case KW_AND:
                        et_push_operation_type(&et, OPERATOR_LOGICAL_AND, tok.loc);
                        break;
                    case KW_OR:
                        et_push_operation_type(&et, OPERATOR_LOGICAL_OR, tok.loc);
                        break;
                    case KW_NOT:
                        et_push_operation_type(&et, OPERATOR_LOGICAL_NOT, tok.loc);
                        break;
                    case KW_IN:
                        et_push_operation_type(&et, OPERATOR_IN, tok.loc);
                        break;
                    case KW_IS:
                        et_push_operation_type(&et, OPERATOR_IS, tok.loc);
                        break;
                    case KW_FALSE: {
                        Operand operand = {.kind = OPERAND_TOKEN, .token = tok};
                        et_push_operand(&et, operand);
                        break;
                    }
                    case KW_TRUE: {
                        Operand operand = {.kind = OPERAND_TOKEN, .token = tok};
                        et_push_operand(&et, operand);
                        break;
                    }
                    default:
                        syntax_errorf(
                            *parser->scanner->index,
                            *tok.loc,
                            0,
                            "not expecting keyword `%s` here",
                            kw_to_cstr(tok.kw)
                        );
                        break;
                }
            } break;
            case TOK_OPERATOR: {
                if (tok.op == OPERATOR_MINUS &&
                    (et.previous == ET_NONE || et.previous == ET_OPERATION)) {
                    et_push_operation_type(&et, OPERATOR_NEGATIVE, tok.loc);
                }
                else
                    et_push_operation_type(&et, tok.op, tok.loc);
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
                    et_push_operation_type(&et, OPERATOR_CALL, tok.loc);
                    et_push_operand(&et, parse_arguments(parser));
                }
                break;
            }
            case TOK_OPEN_SQUARE: {
                if (parser->previous.type == TOK_OPERATOR || et.operands_count == 0)
                    et_push_operand(&et, parse_sequence_enclosure(parser));
                else {
                    et_push_operation_type(&et, OPERATOR_GET_ITEM, tok.loc);
                    et_push_operand(&et, parse_getitem_arguments(parser));
                }
                break;
            }
            case TOK_OPEN_CURLY: {
                et_push_operand(&et, parse_mapped_enclosure(parser));
                break;
            }
            case TOK_DOT: {
                et_push_operation_type(&et, OPERATOR_GET_ATTR, tok.loc);
                Operand right = {
                    .kind = OPERAND_TOKEN,
                    .token = expect_token_type(parser, TOK_IDENTIFIER)};
                et_push_operand(&et, right);
                break;
            }
            default:
                syntax_error(*parser->scanner->index, *tok.loc, 0, "unexpected token");
        };
    } while (!is_end_of_expression(parser, rule));

    return et_to_expr(&et);
}

static Statement* parse_statement(Parser* parser);

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
    Statement* first_body_stmt = parse_statement(parser);
    if (first_body_stmt->loc.col <= parent_indent)
        syntax_error(
            *parser->scanner->index, first_body_stmt->loc, 2, "expected indentation"
        );
    stmt_vector_append(&vec, first_body_stmt);

    for (;;) {
        consume_newline_tokens(parser);
        Token peek = peek_next_token(parser);
        if (peek.loc->col < first_body_stmt->loc.col) {
            if (peek.loc->col > parent_indent)
                syntax_error(
                    *parser->scanner->index, *peek.loc, 2, "inconsistent indentation"
                );
            break;
        }
        if (peek.loc->col > first_body_stmt->loc.col)
            syntax_error(
                *parser->scanner->index, *peek.loc, 2, "inconsistent indentation"
            );
        stmt_vector_append(&vec, parse_statement(parser));
    }

    return (Block){.stmts_count = vec.count, .stmts = stmt_vector_finalize(&vec)};
}

static ImportPath
parse_import_path(Parser* parser)
{
    SourceStringVector vec = str_vector_init(parser->arena);
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
    SourceStringVector what_vec = str_vector_init(parser->arena);
    SourceStringVector as_vec = str_vector_init(parser->arena);
    TokenType end_of_stmt;

    if (peek_next_token(parser).type == TOK_OPEN_PARENS) {
        discard_next_token(parser);
        Token peek = peek_next_token(parser);
        if (peek.type == TOK_CLOSE_PARENS)
            syntax_error(*parser->scanner->index, *peek.loc, 0, "empty import statement");
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
        else {
            SourceString str = {0};
            str_vector_append(&as_vec, str);
        }

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
            syntax_error(*parser->scanner->index, *peek.loc, 0, "unexpected token");
    }

    stmt->what = str_vector_finalize(&what_vec);
    stmt->what_count = what_vec.count;
    stmt->as = str_vector_finalize(&as_vec);
}

static TypeInfo
parse_type_hint(Parser* parser)
{
    Token ident_token = expect_token_type(parser, TOK_IDENTIFIER);
    NpType type = cstr_to_python_type(ident_token.value.data);
    ClassStatement* cls = NULL;
    if (type == NPTYPE_OBJECT) {
        Symbol* sym =
            symbol_hm_get(&parser->scope_stack.scopes[0]->hm, ident_token.value);
        if (!sym || sym->kind != SYM_CLASS) {
            type_errorf(
                *parser->scanner->index,
                *ident_token.loc,
                "unknown class `%s`",
                ident_token.value.data
            );
        }
        cls = sym->cls;
    }
    TypeInfo info = {.type = type, .cls = (type == NPTYPE_OBJECT) ? cls : NULL};

    switch (type) {
        case NPTYPE_FUNCTION: {
            expect_token_type(parser, TOK_OPEN_SQUARE);

            info.sig = arena_alloc(parser->arena, sizeof(Signature));

            expect_token_type(parser, TOK_OPEN_SQUARE);
            TypeInfoVector params_vec = typing_vector_init(parser->arena);
            while (peek_next_token(parser).type != TOK_CLOSE_SQUARE) {
                if (params_vec.count > 0) expect_token_type(parser, TOK_COMMA);
                typing_vector_append(&params_vec, parse_type_hint(parser));
            }
            info.sig->types = typing_vector_finalize(&params_vec);
            info.sig->params_count = params_vec.count;
            discard_next_token(parser);  // CLOSE_SQUARE for params
            expect_token_type(parser, TOK_COMMA);

            info.sig->return_type = parse_type_hint(parser);
            Token close_sq = get_next_token(parser);
            if (close_sq.type != TOK_CLOSE_SQUARE) {
                syntax_error(
                    *parser->scanner->index,
                    *close_sq.loc,
                    0,
                    "returning multiple values from functions is not currently "
                    "implemented"
                );
            }
            return info;
        }
        case NPTYPE_TUPLE: {
            info.inner = arena_alloc(parser->arena, sizeof(TypeInfoInner));
            TypeInfoVector vec = typing_vector_init(parser->arena);
            expect_token_type(parser, TOK_OPEN_SQUARE);
            typing_vector_append(&vec, parse_type_hint(parser));
            while (peek_next_token(parser).type != TOK_CLOSE_SQUARE) {
                expect_token_type(parser, TOK_COMMA);
                typing_vector_append(&vec, parse_type_hint(parser));
            }
            discard_next_token(parser);
            info.inner->types = typing_vector_finalize(&vec);
            info.inner->count = vec.count;
            return info;
        }
        case NPTYPE_LIST: {
            info.inner = arena_alloc(parser->arena, sizeof(TypeInfoInner));
            TypeInfoVector vec = typing_vector_init(parser->arena);
            expect_token_type(parser, TOK_OPEN_SQUARE);
            typing_vector_append(&vec, parse_type_hint(parser));
            expect_token_type(parser, TOK_CLOSE_SQUARE);
            info.inner->types = typing_vector_finalize(&vec);
            info.inner->count = vec.count;
            return info;
        }
        case NPTYPE_DICT: {
            info.inner = arena_alloc(parser->arena, sizeof(TypeInfoInner));
            TypeInfoVector vec = typing_vector_init(parser->arena);
            expect_token_type(parser, TOK_OPEN_SQUARE);
            typing_vector_append(&vec, parse_type_hint(parser));
            expect_token_type(parser, TOK_COMMA);
            typing_vector_append(&vec, parse_type_hint(parser));
            expect_token_type(parser, TOK_CLOSE_SQUARE);
            info.inner->types = typing_vector_finalize(&vec);
            info.inner->count = vec.count;
            return info;
        }
        default:
            return info;
    }
    return info;
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
        if (peek.type == TOK_KEYWORD && peek.kw == KW_ELIF && peek.loc->col == indent) {
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
    if (peek.type == TOK_KEYWORD && peek.kw == KW_ELSE && peek.loc->col == indent) {
        discard_next_token(parser);
        expect_token_type(parser, TOK_COLON);
        conditional->else_body = parse_block(parser, indent);
    }
    return conditional;
}

static void namespace_variable(
    Parser* parser, Variable* variable, SourceString identifier
);

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
        if (begin.loc->col != indent)
            syntax_error(
                *parser->scanner->index, *begin.loc, 2, "inconsistent indentation"
            );
        // we will need a str vector for exception names
        SourceStringVector exceptions = str_vector_init(parser->arena);
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
            syntax_errorf(
                *parser->scanner->index,
                *peek.loc,
                0,
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
            Token as_ident = expect_token_type(parser, TOK_IDENTIFIER);
            except.as = as_ident.value;
            Symbol sym = {
                .kind = SYM_VARIABLE,
                .variable = arena_alloc(parser->arena, sizeof(Variable)),
                .identifier = as_ident.value,
            };
            *sym.variable = (Variable){.kind = VAR_SEMI_SCOPED};
            symbol_hm_put(&scope_stack_peek(&parser->scope_stack)->hm, sym);
        }
        else {
            except.as.length = 0;
            except.as.data = NULL;
        }
        expect_token_type(parser, TOK_COLON);
        // parse body;
        except.body = parse_block(parser, indent);
        // accumulate except statements and consider repeating
        except_vector_append(&excepts, except);
        consume_newline_tokens(parser);
        peek = peek_next_token(parser);
        if (peek.loc->col < indent || peek.type != TOK_KEYWORD || peek.kw != KW_EXCEPT)
            break;
    }
    try_stmt->excepts = except_vector_finalize(&excepts);
    try_stmt->excepts_count = excepts.count;
    if (peek.loc->col < indent) return try_stmt;
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

static void
validate_object_model_signature(
    Parser* parser, Location loc, ObjectModel om, Signature sig
)
{
    // NOTE: being a method the first param `self` is already validated

    switch (om) {
        case OBJECT_MODEL_BOOL:
            if (sig.return_type.type != NPTYPE_BOOL)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __bool__ to return type: `bool`"
                );
            break;
        case OBJECT_MODEL_INIT:
            if (sig.return_type.type != NPTYPE_NONE)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __init__ to return type: `None`"
                );
            break;
        case OBJECT_MODEL_STR:
            if (sig.return_type.type != NPTYPE_STRING)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __str__ to return type: `str`"
                );
            if (sig.params_count > 0)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __str__ to have a single param: `self`"
                );
            break;
        case OBJECT_MODEL_REPR:
            if (sig.return_type.type != NPTYPE_STRING)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __repr__ to return type: `str`"
                );
            if (sig.params_count > 0)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __repr__ to have a single param: `self`"
                );
            break;
        case OBJECT_MODEL_GETITEM:
            if (sig.params_count != 1)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __getitem__ to have 2 params: `self` and `key`"
                );
            break;
        case OBJECT_MODEL_SETITEM:
            if (sig.params_count != 2)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __setitem__ to have 3 params: `self`, `key` and "
                    "`value`"
                );
            break;
        case OBJECT_MODEL_DELITEM:
            if (sig.params_count != 1)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __delitem__ to have 2 params: `self` and `key`"
                );
            break;
        case OBJECT_MODEL_ITER:
            // TODO: some general iterable interface
            if (sig.return_type.type != NPTYPE_ITER)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __iter__ to return type: `NpIter` (general "
                    "iterable "
                    "interface not yet implemented)"
                );
            if (sig.params_count != 0)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __iter__ to have a single param: `self`"
                );
            break;
        case OBJECT_MODEL_NEXT:
            UNIMPLEMENTED("__next__ is not currently implemented");
        case OBJECT_MODEL_LEN:
            if (sig.return_type.type != NPTYPE_INT)
                type_error(
                    *parser->scanner->index, loc, "expecting __len__ to return type `int`"
                );
            if (sig.params_count != 0)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __len__ to have a single param: `self`"
                );
            break;
        case OBJECT_MODEL_CONTAINS:
            if (sig.params_count != 1)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __contains__ to have 2 params: `self` and `item`"
                );
            break;
        case OBJECT_MODEL_HASH:
            if (sig.return_type.type != NPTYPE_INT)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __hash__ to return type: `int`"
                );
            if (sig.params_count != 0)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __hash__ to have a single param: `self`"
                );
            break;
        case OBJECT_MODEL_CALL:
            break;
        case OBJECT_MODEL_LT:
            if (sig.return_type.type != NPTYPE_BOOL)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __lt__ to return type: `bool`"
                );
            if (sig.params_count != 1)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __lt__ to have 2 params: `self` and `other`"
                );
            break;
        case OBJECT_MODEL_LE:
            if (sig.return_type.type != NPTYPE_BOOL)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __le__ to return type: `bool`"
                );
            if (sig.params_count != 1)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __le__ to have 2 params: `self` and `other`"
                );
            break;
        case OBJECT_MODEL_EQ:
            if (sig.return_type.type != NPTYPE_BOOL)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __eq__ to return type: `bool`"
                );
            if (sig.params_count != 1)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __eq__ to have 2 params: `self` and `other`"
                );
            break;
        case OBJECT_MODEL_NE:
            if (sig.return_type.type != NPTYPE_BOOL)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __ne__ to return type: `bool`"
                );
            if (sig.params_count != 1)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __ne__ to have 2 params: `self` and `other`"
                );
            break;
        case OBJECT_MODEL_GT:
            if (sig.return_type.type != NPTYPE_BOOL)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __gt__ to return type: `bool`"
                );
            if (sig.params_count != 1)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __gt__ to have 2 params: `self` and `other`"
                );
            break;
        case OBJECT_MODEL_GE:
            if (sig.return_type.type != NPTYPE_BOOL)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __ge__ to return type: `bool`"
                );
            if (sig.params_count != 1)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __ge__ to have 2 params: `self` and `other`"
                );
            break;
        case OBJECT_MODEL_ADD:
            if (sig.params_count != 1)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __add__ to have 2 params: `self` and `other`"
                );
            break;
        case OBJECT_MODEL_SUB:
            if (sig.params_count != 1)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __sub__ to have 2 params: `self` and `other`"
                );
            break;
        case OBJECT_MODEL_MUL:
            if (sig.params_count != 1)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __mul__ to have 2 params: `self` and `other`"
                );
            break;
        case OBJECT_MODEL_TRUEDIV:
            if (sig.params_count != 1)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __truefiv__ to have 2 params: `self` and `other`"
                );
            break;
        case OBJECT_MODEL_FLOORDIV:
            if (sig.params_count != 1)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __floordiv__ to have 2 params: `self` and `other`"
                );
            break;
        case OBJECT_MODEL_DIVMOD:
            if (sig.params_count != 1)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __divmod__ to have 2 params: `self` and `other`"
                );
            break;
        case OBJECT_MODEL_MOD:
            if (sig.params_count != 1)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __mod__ to have 2 params: `self` and `other`"
                );
            break;
        case OBJECT_MODEL_POW:
            if (sig.params_count != 1)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __pow__ to have 2 params: `self` and `other`"
                );
            break;
        case OBJECT_MODEL_LSHIFT:
            if (sig.params_count != 1)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __lshift__ to have 2 params: `self` and `other`"
                );
            break;
        case OBJECT_MODEL_RSHIFT:
            if (sig.params_count != 1)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __rshift__ to have 2 params: `self` and `other`"
                );
            break;
        case OBJECT_MODEL_AND:
            if (sig.params_count != 1)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __and__ to have 2 params: `self` and `other`"
                );
            break;
        case OBJECT_MODEL_OR:
            if (sig.params_count != 1)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __or__ to have 2 params: `self` and `other`"
                );
            break;
        case OBJECT_MODEL_XOR:
            if (sig.params_count != 1)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __xor__ to have 2 params: `self` and `other`"
                );
            break;
        case OBJECT_MODEL_RADD:
            if (sig.params_count != 1)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __radd__ to have 2 params: `self` and `other`"
                );
            break;
        case OBJECT_MODEL_RSUB:
            if (sig.params_count != 1)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __rsub__ to have 2 params: `self` and `other`"
                );
            break;
        case OBJECT_MODEL_RMUL:
            if (sig.params_count != 1)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __rmul__ to have 2 params: `self` and `other`"
                );
            break;
        case OBJECT_MODEL_RTRUEDIV:
            if (sig.params_count != 1)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __rtruediv__ to have 2 params: `self` and `other`"
                );
            break;
        case OBJECT_MODEL_RFLOORDIV:
            if (sig.params_count != 1)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __rfloordiv__ to have 2 params: `self` and "
                    "`other`"
                );
            break;
        case OBJECT_MODEL_RMOD:
            if (sig.params_count != 1)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __rmod__ to have 2 params: `self` and `other`"
                );
            break;
        case OBJECT_MODEL_RPOW:
            if (sig.params_count != 1)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __rpow__ to have 2 params: `self` and `other`"
                );
            break;
        case OBJECT_MODEL_RLSHIFT:
            if (sig.params_count != 1)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __rlshift__ to have 2 params: `self` and `other`"
                );
            break;
        case OBJECT_MODEL_RRSHIFT:
            if (sig.params_count != 1)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __rrshift__ to have 2 params: `self` and `other`"
                );
            break;
        case OBJECT_MODEL_RAND:
            if (sig.params_count != 1)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __rand__ to have 2 params: `self` and `other`"
                );
            break;
        case OBJECT_MODEL_ROR:
            if (sig.params_count != 1)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __ror__ to have 2 params: `self` and `other`"
                );
            break;
        case OBJECT_MODEL_RXOR:
            if (sig.params_count != 1)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __rxor__ to have 2 params: `self` and `other`"
                );
            break;
        case OBJECT_MODEL_IADD:
            if (sig.params_count != 1)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __iadd__ to have 2 params: `self` and `other`"
                );
            break;
        case OBJECT_MODEL_ISUB:
            if (sig.params_count != 1)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __isub__ to have 2 params: `self` and `other`"
                );
            break;
        case OBJECT_MODEL_IMUL:
            if (sig.params_count != 1)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __imul__ to have 2 params: `self` and `other`"
                );
            break;
        case OBJECT_MODEL_ITRUEDIV:
            if (sig.params_count != 1)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __itruediv__ to have 2 params: `self` and `other`"
                );
            break;
        case OBJECT_MODEL_IFLOORDIV:
            if (sig.params_count != 1)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __ifloordiv__ to have 2 params: `self` and "
                    "`other`"
                );
            break;
        case OBJECT_MODEL_IMOD:
            if (sig.params_count != 1)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __imod__ to have 2 params: `self` and `other`"
                );
            break;
        case OBJECT_MODEL_IPOW:
            if (sig.params_count != 1)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __ipow__ to have 2 params: `self` and `other`"
                );
            break;
        case OBJECT_MODEL_ILSHIFT:
            if (sig.params_count != 1)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __ilshift__ to have 2 params: `self` and `other`"
                );
            break;
        case OBJECT_MODEL_IRSHIFT:
            if (sig.params_count != 1)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __irshift__ to have 2 params: `self` and `other`"
                );
            break;
        case OBJECT_MODEL_IAND:
            if (sig.params_count != 1)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __iand__ to have 2 params: `self` and `other`"
                );
            break;
        case OBJECT_MODEL_IOR:
            if (sig.params_count != 1)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __ior__ to have 2 params: `self` and `other`"
                );
            break;
        case OBJECT_MODEL_IXOR:
            if (sig.params_count != 1)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __ixor__ to have 2 params: `self` and `other`"
                );
            break;
        case OBJECT_MODEL_NEG:
            if (sig.params_count != 0)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __neg__ to have 1 param: `self`"
                );
            break;
        case OBJECT_MODEL_ABS:
            if (sig.params_count != 0)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __abs__ to have 1 param: `self`"
                );
            break;
        case OBJECT_MODEL_INVERT:
            if (sig.params_count != 0)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __invert__ to have 1 param: `self`"
                );
            break;
        case OBJECT_MODEL_INT:
            if (sig.return_type.type != NPTYPE_INT)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __int__ to return type: `int`"
                );
            if (sig.params_count != 0)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __int__ to have 1 param: `self`"
                );
            break;
        case OBJECT_MODEL_FLOAT:
            if (sig.return_type.type != NPTYPE_FLOAT)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __float__ to return type: `float`"
                );
            if (sig.params_count != 0)
                type_error(
                    *parser->scanner->index,
                    loc,
                    "expecting __float__ to have 1 param: `self`"
                );
            break;
        case OBJECT_MODEL_ROUND:
            UNIMPLEMENTED("__round__ is not yet implemented");
            break;
        case OBJECT_MODEL_TRUNC:
            UNIMPLEMENTED("__trunc__ is not yet implemented");
            break;
        case OBJECT_MODEL_FLOOR:
            UNIMPLEMENTED("__floor__ is not yet implemented");
            break;
        case OBJECT_MODEL_CEIL:
            UNIMPLEMENTED("__ceil__ is not yet implemented");
            break;
        case OBJECT_MODEL_ENTER:
            UNIMPLEMENTED("__enter__ is not yet implemented");
            break;
        case OBJECT_MODEL_EXIT:
            UNIMPLEMENTED("__exit__ is not yet implemented");
            break;
        default:
            UNREACHABLE();
    }
}

static void
namespace_method(Parser* parser, FunctionStatement* fndef, ClassStatement* clsdef)
{
    fndef->ns_ident.length =
        fndef->name.length + parser->file_namespace_length + clsdef->name.length + 1;
    fndef->ns_ident.data = arena_alloc(parser->arena, fndef->ns_ident.length + 1);
    char* write = (char*)fndef->ns_ident.data;
    memcpy(write, parser->file_namespace, parser->file_namespace_length);
    write += parser->file_namespace_length;
    memcpy(write, clsdef->name.data, clsdef->name.length);
    write += clsdef->name.length;
    *write++ = '_';
    memcpy(write, fndef->name.data, fndef->name.length);
}

static void
namespace_function(Parser* parser, FunctionStatement* fndef)
{
    fndef->ns_ident.length = fndef->name.length + parser->file_namespace_length;
    fndef->ns_ident.data = arena_alloc(parser->arena, fndef->ns_ident.length + 1);
    char* write = (char*)fndef->ns_ident.data;
    memcpy(write, parser->file_namespace, parser->file_namespace_length);
    write += parser->file_namespace_length;
    memcpy(write, fndef->name.data, fndef->name.length);
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
    SourceStringVector params = str_vector_init(parser->arena);
    TypeInfoVector types = typing_vector_init(parser->arena);
    ExpressionVector defaults = expr_vector_init(parser->arena);

    LexicalScope* fn_scope = scope_init(parser->arena);
    LexicalScope* parent_scope = scope_stack_peek(&parser->scope_stack);

    SourceString self_param;
    TypeInfo self_type;

    switch (parent_scope->kind) {
        case SCOPE_FUNCTION:
            parent_scope->kind = SCOPE_CLOSURE_PARENT;
            fn_scope->kind = SCOPE_CLOSURE_CHILD;
            break;
        case SCOPE_CLOSURE_CHILD:
            fn_scope->kind = SCOPE_CLOSURE_CHILD;
            break;
        case SCOPE_CLOSURE_PARENT:
            fn_scope->kind = SCOPE_CLOSURE_CHILD;
            break;
        case SCOPE_TOP:
            fn_scope->kind = SCOPE_FUNCTION;
            break;
        case SCOPE_CLASS: {
            fn_scope->kind = SCOPE_FUNCTION;
            func->is_method = true;
            // parse name of self param
            Token self_token = get_next_token(parser);
            if (self_token.type != TOK_IDENTIFIER) {
                syntax_error(
                    *parser->scanner->index,
                    *self_token.loc,
                    1,
                    "expecting `self` param for method def"
                );
            }
            self_param = self_token.value;
            self_type = (TypeInfo){.type = NPTYPE_OBJECT, .cls = parent_scope->cls};
            break;
        }
    }

    Token peek = peek_next_token(parser);

    while (peek.type != TOK_CLOSE_PARENS) {
        if (func->is_method || params.count > 0) expect_token_type(parser, TOK_COMMA);
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
            syntax_error(
                *parser->scanner->index,
                *param.loc,
                1,
                "non default argument follows default argument"
            );
        }
    }
    discard_next_token(parser);  // close parens

    TypeInfo return_type = {.type = NPTYPE_NONE};
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
    fn_scope->func = func;
    if (func->is_method) {
        Variable* local_var = arena_alloc(parser->arena, sizeof(Variable));
        local_var->kind = VAR_SELF;
        local_var->identifier = self_param;
        local_var->compiled_name = self_param;
        local_var->type_info = self_type;
        Symbol local_sym = {.kind = SYM_VARIABLE, .variable = local_var};
        local_sym.identifier = self_param;
        symbol_hm_put(&fn_scope->hm, local_sym);
    }
    for (size_t i = 0; i < sig.params_count; i++) {
        Variable* local_var = arena_alloc(parser->arena, sizeof(Variable));
        local_var->kind = VAR_ARGUMENT;
        local_var->identifier = sig.params[i];
        local_var->compiled_name =
            sig.params[i];  // function locals don't need name mangling
        local_var->type_info = sig.types[i];
        Symbol local_sym = {.kind = SYM_VARIABLE, .variable = local_var};
        local_sym.identifier = sig.params[i];
        symbol_hm_put(&fn_scope->hm, local_sym);
    }

    // parse body
    scope_stack_push(&parser->scope_stack, fn_scope);
    func->scope = fn_scope;
    func->body = parse_block(parser, loc.col);
    scope_stack_pop(&parser->scope_stack);
    symbol_hm_finalize(&fn_scope->hm);

    Symbol sym = {.kind = SYM_FUNCTION, .func = func, .identifier = func->name};
    if (parent_scope->kind == SCOPE_CLASS) {
        namespace_method(parser, func, parent_scope->cls);
        // if function is part of the python object model validate it's signature
        // and add it to the classes object model table
        ObjectModel om = source_string_to_object_model(func->name);
        if (om != NOT_IN_OBJECT_MODEL) {
            validate_object_model_signature(parser, loc, om, func->sig);
            parent_scope->cls->object_model_methods[om] = func;
        }
    }
    else
        namespace_function(parser, func);

    symbol_hm_put(&parent_scope->hm, sym);
    return func;
}

static void
namespace_class(Parser* parser, ClassStatement* clsdef)
{
    clsdef->ns_ident.length = clsdef->name.length + parser->file_namespace_length;
    clsdef->ns_ident.data = arena_alloc(parser->arena, clsdef->ns_ident.length + 1);
    memcpy(
        (char*)clsdef->ns_ident.data,
        parser->file_namespace,
        parser->file_namespace_length
    );
    memcpy(
        (char*)clsdef->ns_ident.data + parser->file_namespace_length,
        clsdef->name.data,
        clsdef->name.length
    );
}

static ClassStatement*
init_class_statement(Parser* parser, SourceString name)
{
    ClassStatement* cls = arena_alloc(parser->arena, sizeof(ClassStatement));
    LexicalScope* scope = scope_init(parser->arena);
    scope->kind = SCOPE_CLASS;
    scope->cls = cls;
    cls->scope = scope;
    cls->name = name;
    namespace_class(parser, cls);
    parser->current_class_members_count = 0;
    parser->current_class_members_capacity = 8;
    parser->current_class_members = arena_dynamic_alloc(
        parser->arena,
        sizeof(AnnotationStatement) * parser->current_class_members_capacity
    );
    Symbol sym = {.kind = SYM_CLASS, .cls = cls};
    sym.identifier = cls->name;
    symbol_hm_put(&scope_stack_peek(&parser->scope_stack)->hm, sym);
    scope_stack_push(&parser->scope_stack, cls->scope);
    return cls;
}

static void
add_class_member(Parser* parser, AnnotationStatement* member, Location loc)
{
    if (parser->current_class_members_count == parser->current_class_members_capacity) {
        parser->current_class_members_capacity *= 2;
        parser->current_class_members = arena_dynamic_realloc(
            parser->arena,
            parser->current_class_members,
            sizeof(AnnotationStatement) * parser->current_class_members_capacity
        );
    }
    if (!member->initial && parser->current_class_members_defaults_count > 0) {
        syntax_error(
            *parser->scanner->index,
            loc,
            0,
            "class members without default values must be declared before all "
            "members "
            "with default values"
        );
    }
    if (member->initial) parser->current_class_members_defaults_count += 1;
    parser->current_class_members[parser->current_class_members_count++] = *member;
}

static void
finalize_class_statement(Parser* parser, ClassStatement* cls)
{
    cls->sig.params_count = parser->current_class_members_count;
    cls->sig.params = arena_alloc(
        parser->arena, sizeof(SourceString) * parser->current_class_members_count
    );
    cls->sig.types = arena_alloc(
        parser->arena, sizeof(TypeInfo) * parser->current_class_members_count
    );
    cls->sig.defaults = arena_alloc(
        parser->arena, sizeof(Expression*) * parser->current_class_members_defaults_count
    );
    cls->sig.return_type.type = NPTYPE_OBJECT;
    cls->sig.return_type.cls = cls;

    for (size_t i = 0; i < parser->current_class_members_count; i++) {
        AnnotationStatement annotation = parser->current_class_members[i];
        cls->sig.params[i] = annotation.identifier;
        cls->sig.types[i] = annotation.type;
        Symbol sym = {.kind = SYM_MEMBER, .member_type = &cls->sig.types[i]};
        sym.identifier = annotation.identifier;
        symbol_hm_put(&cls->scope->hm, sym);

        if (annotation.initial)
            cls->sig.defaults[cls->sig.defaults_count++] = annotation.initial;
    }

    arena_dynamic_free(parser->arena, parser->current_class_members);
    parser->current_class_members_capacity = 0;
    parser->current_class_members_count = 0;
    parser->current_class_members_defaults_count = 0;
    parser->current_class_members = NULL;

    scope_stack_pop(&parser->scope_stack);
}

static ClassStatement*
parse_class_statement(Parser* parser, unsigned int indent)
{
    if (scope_stack_peek(&parser->scope_stack)->kind != SCOPE_TOP) {
        syntax_error(
            *parser->scanner->index,
            *get_next_token(parser).loc,
            0,
            "class definitions currenly only allowed at top level scope"
        );
    }
    discard_next_token(parser);
    SourceString cls_name = expect_token_type(parser, TOK_IDENTIFIER).value;
    ClassStatement* cls = init_class_statement(parser, cls_name);

    if (peek_next_token(parser).type == TOK_OPEN_PARENS) {
        discard_next_token(parser);
        cls->base = expect_token_type(parser, TOK_IDENTIFIER).value;
        expect_token_type(parser, TOK_CLOSE_PARENS);
    }
    expect_token_type(parser, TOK_COLON);
    cls->body = parse_block(parser, indent);

    finalize_class_statement(parser, cls);
    return cls;
}

static void
namespace_variable(Parser* parser, Variable* variable, SourceString identifier)
{
    variable->compiled_name.length = identifier.length + parser->file_namespace_length;
    variable->compiled_name.data =
        arena_alloc(parser->arena, variable->compiled_name.length + 1);
    memcpy(
        (char*)variable->compiled_name.data,
        parser->file_namespace,
        parser->file_namespace_length
    );
    memcpy(
        (char*)variable->compiled_name.data + parser->file_namespace_length,
        identifier.data,
        identifier.length
    );
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
        var->kind = VAR_REGULAR;
        var->identifier = assign_to->operands[0].token.value;
        var->type_info = (TypeInfo){.type = NPTYPE_UNTYPED};
        Symbol sym = {
            .kind = SYM_VARIABLE, .variable = var, .identifier = var->identifier};
        namespace_variable(parser, var, sym.identifier);
        symbol_hm_put(&scope->hm, sym);
    }
    return assignment;
}

static AnnotationStatement*
parse_annotation_statement(Parser* parser, Location loc, SourceString identifier)
{
    AnnotationStatement* annotation =
        arena_alloc(parser->arena, sizeof(AnnotationStatement));
    annotation->identifier = identifier;
    discard_next_token(parser);
    annotation->type = parse_type_hint(parser);

    Token peek = peek_next_token(parser);
    if (peek.type != TOK_NEWLINE) {
        if (peek.type != TOK_OPERATOR && peek.op != OPERATOR_ASSIGNMENT)
            syntax_error(
                *parser->scanner->index, *peek.loc, 0, "expecting either `newline` or `=`"
            );
        discard_next_token(parser);
        annotation->initial = parse_expression(parser);
        expect_token_type(parser, TOK_NEWLINE);
    }

    LexicalScope* scope = scope_stack_peek(&parser->scope_stack);
    if (scope->kind == SCOPE_CLASS)
        add_class_member(parser, annotation, loc);
    else {
        Symbol sym = {
            .kind = SYM_VARIABLE,
            .variable = arena_alloc(parser->arena, sizeof(Variable))};
        sym.identifier = identifier;
        sym.variable->identifier = identifier;
        sym.variable->type_info = annotation->type;
        namespace_variable(parser, sym.variable, sym.identifier);
        symbol_hm_put(&scope->hm, sym);
    }
    return annotation;
}

static void
parse_global(Parser* parser, Location loc)
{
    LexicalScope* scope = scope_stack_peek(&parser->scope_stack);
    if (scope->kind != SCOPE_FUNCTION) {
        syntax_error(
            *parser->scanner->index,
            loc,
            0,
            "`global` keyword must be used in the context of a function"
        );
    }

    Token ident = expect_token_type(parser, TOK_IDENTIFIER);
    expect_token_type(parser, TOK_NEWLINE);

    Symbol* local_sym = symbol_hm_get(&scope->hm, ident.value);
    if (local_sym) {
        name_errorf(
            *parser->scanner->index, loc, "`%s` identifier already declared as a local"
        );
    }
    Symbol* global_sym = symbol_hm_get(&parser->scope_stack.scopes[0]->hm, ident.value);
    if (!global_sym) {
        name_errorf(
            *parser->scanner->index, loc, "`%s` identifier not found in global scope"
        );
    }

    Symbol placeholder_local = {
        .kind = SYM_GLOBAL, .identifier = ident.value, .globalvar = global_sym->variable};
    symbol_hm_put(&scope->hm, placeholder_local);
}

static AssertStatement*
parse_assert(Parser* parser)
{
    AssertStatement* assert_ = arena_alloc(parser->arena, sizeof(AssertStatement));
    assert_->expr = parse_expression(parser);
    assert_->source_code = get_assert_source_code(parser->scanner);
    return assert_;
}

static Statement*
parse_statement(Parser* parser)
{
    ConsumableParserRule rule = consume_rule(parser);
    Statement* stmt = arena_alloc(parser->arena, sizeof(Statement));

    consume_newline_tokens(parser);
    Token peek = peek_next_token(parser);
    stmt->loc = *peek.loc;
    const char* error =
        indent_check(&parser->indent_stack, stmt->loc, rule == BLOCK_BEGIN);
    if (error) syntax_error(*parser->scanner->index, stmt->loc, 2, error);

    if (peek.type == TOK_KEYWORD) {
        switch (peek.kw) {
            case KW_GLOBAL: {
                discard_next_token(parser);
                parse_global(parser, stmt->loc);
                return parse_statement(parser);
            }
            case KW_BREAK:
                // TODO: make sure that we're currently parsing some kind of loop
                discard_next_token(parser);
                stmt->kind = STMT_BREAK;
                return stmt;
            case KW_CONTINUE:
                // TODO: make sure that we're currently parsing some kind of loop
                discard_next_token(parser);
                stmt->kind = STMT_CONTINUE;
                return stmt;
            case KW_RETURN: {
                discard_next_token(parser);
                stmt->kind = STMT_RETURN;
                stmt->return_expr = parse_expression(parser);
                return stmt;
            }
            case KW_ASSERT:
                discard_next_token(parser);
                stmt->kind = STMT_ASSERT;
                stmt->assert_ = parse_assert(parser);
                return stmt;
            case KW_FOR: {
                stmt->kind = STMT_FOR_LOOP;
                stmt->for_loop = parse_for_loop(parser, stmt->loc.col);
                return stmt;
            }
            case KW_PASS:
                discard_next_token(parser);
                expect_token_type(parser, TOK_NEWLINE);
                stmt->kind = STMT_NO_OP;
                return stmt;
            case KW_WHILE: {
                stmt->kind = STMT_WHILE;
                stmt->while_loop = parse_while_loop(parser, stmt->loc.col);
                return stmt;
            }
            case KW_IMPORT: {
                stmt->kind = STMT_IMPORT;
                stmt->import = parse_import_statement(parser);
                return stmt;
            }
            case KW_FROM: {
                discard_next_token(parser);
                stmt->kind = STMT_IMPORT;
                stmt->import = arena_alloc(parser->arena, sizeof(ImportStatement));
                stmt->import->from = parse_import_path(parser);
                expect_keyword(parser, KW_IMPORT);
                parse_import_group(parser, stmt->import);
                return stmt;
            }
            case KW_IF: {
                stmt->kind = STMT_IF;
                stmt->conditional = parse_if_statement(parser, stmt->loc.col);
                return stmt;
            }
            case KW_TRY: {
                stmt->kind = STMT_TRY;
                stmt->try_stmt = parse_try_statement(parser, stmt->loc.col);
                return stmt;
            }
            case KW_WITH: {
                stmt->kind = STMT_WITH;
                stmt->with = parse_with_statement(parser, stmt->loc.col);
                return stmt;
            }
            case KW_DEF: {
                stmt->kind = STMT_FUNCTION;
                stmt->func = parse_function_statement(parser, stmt->loc);
                return stmt;
            }
            case KW_CLASS: {
                stmt->kind = STMT_CLASS;
                stmt->cls = parse_class_statement(parser, stmt->loc.col);
                return stmt;
            }
            default:
                break;
        }
    }
    if (peek.type == TOK_EOF) {
        discard_next_token(parser);
        stmt->kind = STMT_EOF;
        return stmt;
    }
    if (peek.type == TOK_DECORATOR) {
        discard_next_token(parser);
        Expression* decorator = parse_expression(parser);
        Statement* stmt = parse_statement(parser);
        if (stmt->kind != STMT_FUNCTION)
            syntax_error(
                *parser->scanner->index, stmt->loc, 1, "expecting function definition"
            );
        stmt->func->decorator = decorator;
        return stmt;
    }

    Expression* expr = parse_expression(parser);

    switch (peek_next_token(parser).type) {
        case TOK_OPERATOR:
            stmt->kind = STMT_ASSIGNMENT;
            stmt->assignment = parse_assignment_statement(parser, expr);
            break;
        case TOK_COLON:
            assert(
                expr->operations_count == 0 &&
                "annotations left expresson should just be an identifier"
            );
            stmt->kind = STMT_ANNOTATION;
            stmt->annotation = parse_annotation_statement(
                parser, stmt->loc, expr->operands[0].token.value
            );
            break;
        default:
            stmt->kind = STMT_EXPR;
            stmt->expr = expr;
    }
    return stmt;
}

#define LEXER_STATEMENTS_CHUNK_SIZE 64

Lexer
lex_file(const char* filepath)
{
    FILE* file = fopen(filepath, "r");
    if (!file) errorf("failed to open (%s) for reading: %s", filepath, strerror(errno));
    Arena* arena = arena_init();
    const char* ns = file_namespace(arena, filepath);
    size_t ns_len = strlen(ns);
    Lexer lexer = {
        .arena = arena,
        .top_level = scope_init(arena),
        .file_namespace = ns,
        .file_namespace_length = ns_len,
        .index = create_file_index(arena, filepath)};
    Location start_location = {.line = 1, .filepath = filepath};
    TokenQueue tq = {0};
    Scanner scanner = {
        .arena = arena,
        .loc = start_location,
        .srcfile = file,
        .index = &lexer.index,
        .tq = &tq};
    Parser parser = {
        .arena = arena,
        .scanner = &scanner,
        .tq = &tq,
        .file_namespace = ns,
        .file_namespace_length = ns_len};
    scope_stack_push(&parser.scope_stack, lexer.top_level);

    size_t statements_capacity = LEXER_STATEMENTS_CHUNK_SIZE;
    lexer.statements = malloc(sizeof(Statement) * statements_capacity);
    if (!lexer.statements) error("out of memory");

    do {
        if (lexer.n_statements >= statements_capacity) {
            statements_capacity += LEXER_STATEMENTS_CHUNK_SIZE;
            lexer.statements =
                realloc(lexer.statements, sizeof(Statement) * statements_capacity);
            if (!lexer.statements) error("out of memory");
        }
        lexer.statements[lexer.n_statements] = parse_statement(&parser);
    } while (lexer.statements[lexer.n_statements++]->kind != STMT_EOF);
    symbol_hm_finalize(&lexer.top_level->hm);

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
    if (!file) errorf("failed to open (%s) for reading: %s", filepath, strerror(errno));

    Location start_location = {.line = 1, .filepath = filepath};
    TokenQueue tq = {0};
    Arena* arena = arena_init();
    FileIndex index = create_file_index(arena, filepath);
    Scanner scanner = {
        .arena = arena,
        .loc = start_location,
        .srcfile = file,
        .tq = &tq,
        .index = &index};

    size_t count = 0;
    size_t capacity = 64;
    Token* tokens = malloc(sizeof(Token) * capacity);
    if (!tokens) error("out of memory");

    do {
        if (count == capacity) {
            capacity *= 2;
            tokens = realloc(tokens, sizeof(Token) * capacity);
            if (!tokens) error("out of memory");
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
