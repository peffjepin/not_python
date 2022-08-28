#ifndef DEBUG_COMMON_H
#define DEBUG_COMMON_H

#include "../src/compiler_types.h"
#include "../src/lexer.h"
#include "../src/lexer_types.h"

void print_token(Token tok);
void print_token_type(TokenType type);
void print_operator_enum(Operator op);
void print_keyword(Keyword kw);
void print_statement(Statement* stmt);

#endif
