#include "tokens.h"

#include <stdbool.h>

static bool
token_stream_full(TokenStream* ts)
{
    if (ts->write_head < ts->read_head) return (ts->read_head - ts->write_head == 1);
    return (ts->read_head == 0 && ts->write_head == TOKEN_STREAM_CAPACITY - 1);
}

int
token_stream_write(TokenStream* ts, Token tok)
{
    if (token_stream_full(ts)) return -1;
    ts->tokens[ts->write_head++] = tok;
    if (ts->write_head == TOKEN_STREAM_CAPACITY) ts->write_head = 0;
    return 0;
}

Token
token_stream_consume(TokenStream* ts)
{
    Token tok = {0};
    if (ts->read_head == ts->write_head) return tok;
    tok = ts->tokens[ts->read_head];
    ts->tokens[ts->read_head].type = NULL_TOKEN;
    if (ts->read_head == TOKEN_STREAM_CAPACITY - 1) ts->read_head = 0;
    else ts->read_head++;
    return tok;
}

Token
token_stream_peek(TokenStream* ts)
{
    Token null_tok = {0};
    if (ts->read_head == ts->write_head) return null_tok;
    return ts->tokens[ts->read_head];
}

Token
token_stream_peekn(TokenStream* ts, size_t n)
{
    Token null_tok = {0};
    size_t read_index = (ts->read_head + n) % TOKEN_STREAM_CAPACITY;

    // boundary checks
    if (read_index == ts->write_head) return null_tok;
    if (ts->read_head > ts->write_head && read_index > ts->write_head &&
        read_index < ts->read_head)
        return null_tok;
    else if (read_index < ts->read_head || read_index > ts->write_head)
        return null_tok;

    return ts->tokens[read_index];
}
