#ifndef TYPE_CHECKER_H
#define TYPE_CHECKER_H

#include "syntax.h"

typedef struct {
    LexicalScope* locals;
    LexicalScope* globals;
    Arena* arena;
} TypeChecker;

TypeInfo resolve_operand_type(TypeChecker* tc, Operand operand);
TypeInfo resolve_operation_type(TypeInfo left, TypeInfo right, Operator op_type);
bool compare_types(TypeInfo type1, TypeInfo type2);

#endif
