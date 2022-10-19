#ifndef TYPE_CHECKER_H
#define TYPE_CHECKER_H

#include "lexer_helpers.h"
#include "syntax.h"

typedef struct {
    LexicalScopeStack* stack;
    Arena* arena;
} TypeChecker;

TypeInfo resolve_operand_type(TypeChecker* tc, Operand operand);
TypeInfo resolve_operation_type(TypeInfo left, TypeInfo right, Operator op_type);
bool compare_types(TypeInfo type1, TypeInfo type2);
FunctionStatement* find_object_op_function(
    TypeInfo left, TypeInfo right, Operator op, bool* is_rop, bool* is_unary
);

#endif
