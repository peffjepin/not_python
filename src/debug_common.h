#ifndef DEBUG_COMMON_H
#define DEBUG_COMMON_H

#include "generated.h"
#include "lexer.h"
#include "syntax.h"

void print_token(Token tok);
void print_token_type(TokenType type);
void print_operator_enum(Operator op);
void print_keyword(Keyword kw);
void print_statement(Statement* stmt, int indent);
void print_scopes(Lexer* lexer);

#endif