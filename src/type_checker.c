#include "type_checker.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "generated.h"
#include "lexer_helpers.h"

#define UNREACHABLE() assert(0 && "unreachable");

static TypeInfo
resolve_literal_type(Token token)
{
    switch (token.type) {
        case TOK_STRING:
            return (TypeInfo){.type = PYTYPE_STRING};
        case TOK_NUMBER: {
            for (size_t i = 0;; i++) {
                // TODO: this could be handled in the scanner
                char c = token.value[i];
                if (c == '\0') return (TypeInfo){.type = PYTYPE_INT};
                if (c == '.' || c == 'f') return (TypeInfo){.type = PYTYPE_FLOAT};
            }
            return (TypeInfo){.type = PYTYPE_INT};
        }
        case TOK_KEYWORD:
            if (token.kw == KW_TRUE || token.kw == KW_FALSE)
                return (TypeInfo){.type = PYTYPE_BOOL};
            break;
        default:
            break;
    }
    return (TypeInfo){.type = PYTYPE_UNTYPED};
}

static bool
compare_inner_type(TypeInfoInner* inner1, TypeInfoInner* inner2)
{
    if (inner1->count != inner2->count) return false;
    for (size_t i = 0; i < inner1->count; i++) {
        if (!compare_types(inner1->types[i], inner2->types[i])) return false;
    }
    return true;
}

bool
compare_types(TypeInfo type1, TypeInfo type2)
{
    bool outer_equal = type1.type == type2.type;
    switch (type1.type) {
        case PYTYPE_LIST:
            return outer_equal && compare_inner_type(type1.inner, type2.inner);
        case PYTYPE_TUPLE:
            return outer_equal && compare_inner_type(type1.inner, type2.inner);
        case PYTYPE_DICT:
            return outer_equal && compare_inner_type(type1.inner, type2.inner);
        case PYTYPE_OBJECT:
            return outer_equal && (strcmp(type1.cls->name, type2.cls->name) == 0);
        default:
            return outer_equal;
    }
    UNREACHABLE();
}

static bool
is_number(TypeInfo type)
{
    return (type.type == PYTYPE_INT || type.type == PYTYPE_FLOAT);
}

#define TYPE_INFO_UNTYPED                                                                \
    {                                                                                    \
        .type = PYTYPE_UNTYPED                                                           \
    }
#define TYPE_INFO_FLOAT                                                                  \
    {                                                                                    \
        .type = PYTYPE_FLOAT                                                             \
    }
#define TYPE_INFO_INT                                                                    \
    {                                                                                    \
        .type = PYTYPE_INT                                                               \
    }
#define TYPE_INFO_NONE                                                                   \
    {                                                                                    \
        .type = PYTYPE_NONE                                                              \
    }
#define TYPE_INFO_STRING                                                                 \
    {                                                                                    \
        .type = PYTYPE_STRING                                                            \
    }
#define TYPE_INFO_LIST                                                                   \
    {                                                                                    \
        .type = PYTYPE_LIST                                                              \
    }
#define TYPE_INFO_DICT                                                                   \
    {                                                                                    \
        .type = PYTYPE_DICT                                                              \
    }
#define TYPE_INFO_TUPLE                                                                  \
    {                                                                                    \
        .type = PYTYPE_TUPLE                                                             \
    }
#define TYPE_INFO_OBJECT                                                                 \
    {                                                                                    \
        .type = PYTYPE_OBJECT                                                            \
    }
#define TYPE_INFO_BOOL                                                                   \
    {                                                                                    \
        .type = PYTYPE_BOOL                                                              \
    }
#define TYPE_INFO_SLICE                                                                  \
    {                                                                                    \
        .type = PYTYPE_SLICE                                                             \
    }

static const bool TYPE_USES_SPECIAL_RESOLUTION_RULES[PYTYPE_COUNT] = {
    [PYTYPE_LIST] = true,
    [PYTYPE_DICT] = true,
    [PYTYPE_OBJECT] = true,
    [PYTYPE_TUPLE] = true,
};

static const bool SPECIAL_OPERATOR_RULES[OPERATORS_MAX] = {
    [OPERATOR_CONDITIONAL_IF] = true,
    [OPERATOR_CONDITIONAL_ELSE] = true,
    [OPERATOR_CALL] = true,
    [OPERATOR_GET_ITEM] = true,
    [OPERATOR_GET_ATTR] = true,
    [OPERATOR_LOGICAL_AND] = true,
    [OPERATOR_LOGICAL_OR] = true,
    [OPERATOR_LOGICAL_NOT] = true,
    [OPERATOR_IN] = true,
    [OPERATOR_IS] = true,
    [OPERATOR_NEGATIVE] = true,
    [OPERATOR_BITWISE_NOT] = true,
};

static const TypeInfo
    OPERATION_TYPE_RESOLUTION_TABLE[OPERATORS_MAX][PYTYPE_COUNT][PYTYPE_COUNT] = {
        [OPERATOR_PLUS] =
            {
                [PYTYPE_INT] =
                    {
                        [PYTYPE_INT] = TYPE_INFO_INT,
                        [PYTYPE_FLOAT] = TYPE_INFO_FLOAT,
                    },
                [PYTYPE_FLOAT] =
                    {
                        [PYTYPE_INT] = TYPE_INFO_FLOAT,
                        [PYTYPE_FLOAT] = TYPE_INFO_FLOAT,
                    },
                [PYTYPE_STRING] =
                    {
                        [PYTYPE_STRING] = TYPE_INFO_STRING,
                    },
            },
        [OPERATOR_MINUS] =
            {
                [PYTYPE_INT] =
                    {
                        [PYTYPE_INT] = TYPE_INFO_INT,
                        [PYTYPE_FLOAT] = TYPE_INFO_FLOAT,
                    },
                [PYTYPE_FLOAT] =
                    {
                        [PYTYPE_INT] = TYPE_INFO_FLOAT,
                        [PYTYPE_FLOAT] = TYPE_INFO_FLOAT,
                    },
            },
        [OPERATOR_MULT] =
            {
                [PYTYPE_INT] =
                    {
                        [PYTYPE_INT] = TYPE_INFO_INT,
                        [PYTYPE_FLOAT] = TYPE_INFO_FLOAT,
                        [PYTYPE_STRING] = TYPE_INFO_STRING,
                    },
                [PYTYPE_FLOAT] =
                    {
                        [PYTYPE_INT] = TYPE_INFO_FLOAT,
                        [PYTYPE_FLOAT] = TYPE_INFO_FLOAT,
                    },
                [PYTYPE_STRING] =
                    {
                        [PYTYPE_INT] = TYPE_INFO_STRING,
                    },
            },
        [OPERATOR_DIV] =
            {
                [PYTYPE_INT] =
                    {
                        [PYTYPE_FLOAT] = TYPE_INFO_FLOAT,
                        [PYTYPE_INT] = TYPE_INFO_FLOAT,
                    },
                [PYTYPE_FLOAT] =
                    {
                        [PYTYPE_FLOAT] = TYPE_INFO_FLOAT,
                        [PYTYPE_INT] = TYPE_INFO_FLOAT,
                    },
            },
        [OPERATOR_MOD] =
            {
                [PYTYPE_INT] =
                    {
                        [PYTYPE_INT] = TYPE_INFO_INT,
                    },
                [PYTYPE_FLOAT] =
                    {
                        [PYTYPE_INT] = TYPE_INFO_FLOAT,
                    },
            },
        [OPERATOR_POW] =
            {
                [PYTYPE_INT] =
                    {
                        [PYTYPE_INT] = TYPE_INFO_INT,
                        [PYTYPE_FLOAT] = TYPE_INFO_FLOAT,
                    },
                [PYTYPE_FLOAT] =
                    {
                        [PYTYPE_INT] = TYPE_INFO_FLOAT,
                        [PYTYPE_FLOAT] = TYPE_INFO_FLOAT,
                    },
            },
        [OPERATOR_FLOORDIV] =
            {
                [PYTYPE_INT] =
                    {
                        [PYTYPE_INT] = TYPE_INFO_INT,
                        [PYTYPE_FLOAT] = TYPE_INFO_INT,
                    },
                [PYTYPE_FLOAT] =
                    {
                        [PYTYPE_INT] = TYPE_INFO_INT,
                        [PYTYPE_FLOAT] = TYPE_INFO_INT,
                    },
            },
        [OPERATOR_EQUAL] =
            {
                [PYTYPE_INT] =
                    {
                        [PYTYPE_INT] = TYPE_INFO_BOOL,
                        [PYTYPE_FLOAT] = TYPE_INFO_BOOL,
                        [PYTYPE_BOOL] = TYPE_INFO_BOOL,
                    },
                [PYTYPE_FLOAT] =
                    {
                        [PYTYPE_INT] = TYPE_INFO_BOOL,
                        [PYTYPE_FLOAT] = TYPE_INFO_BOOL,
                        [PYTYPE_BOOL] = TYPE_INFO_BOOL,
                    },
                [PYTYPE_BOOL] =
                    {
                        [PYTYPE_INT] = TYPE_INFO_BOOL,
                        [PYTYPE_FLOAT] = TYPE_INFO_BOOL,
                        [PYTYPE_BOOL] = TYPE_INFO_BOOL,
                    },
                [PYTYPE_STRING] =
                    {
                        [PYTYPE_STRING] = TYPE_INFO_BOOL,
                    },
            },
        [OPERATOR_NOT_EQUAL] =
            {
                [PYTYPE_INT] =
                    {
                        [PYTYPE_INT] = TYPE_INFO_BOOL,
                        [PYTYPE_FLOAT] = TYPE_INFO_BOOL,
                        [PYTYPE_BOOL] = TYPE_INFO_BOOL,
                    },
                [PYTYPE_FLOAT] =
                    {
                        [PYTYPE_INT] = TYPE_INFO_BOOL,
                        [PYTYPE_FLOAT] = TYPE_INFO_BOOL,
                        [PYTYPE_BOOL] = TYPE_INFO_BOOL,
                    },
                [PYTYPE_BOOL] =
                    {
                        [PYTYPE_INT] = TYPE_INFO_BOOL,
                        [PYTYPE_FLOAT] = TYPE_INFO_BOOL,
                        [PYTYPE_BOOL] = TYPE_INFO_BOOL,
                    },
                [PYTYPE_STRING] =
                    {
                        [PYTYPE_STRING] = TYPE_INFO_BOOL,
                    },
            },
        [OPERATOR_GREATER] =
            {
                [PYTYPE_INT] =
                    {
                        [PYTYPE_INT] = TYPE_INFO_BOOL,
                        [PYTYPE_FLOAT] = TYPE_INFO_BOOL,
                    },
                [PYTYPE_FLOAT] =
                    {
                        [PYTYPE_INT] = TYPE_INFO_BOOL,
                        [PYTYPE_FLOAT] = TYPE_INFO_BOOL,
                    },
                [PYTYPE_STRING] =
                    {
                        [PYTYPE_STRING] = TYPE_INFO_BOOL,
                    },
            },
        [OPERATOR_LESS] =
            {
                [PYTYPE_INT] =
                    {
                        [PYTYPE_INT] = TYPE_INFO_BOOL,
                        [PYTYPE_FLOAT] = TYPE_INFO_BOOL,
                    },
                [PYTYPE_FLOAT] =
                    {
                        [PYTYPE_INT] = TYPE_INFO_BOOL,
                        [PYTYPE_FLOAT] = TYPE_INFO_BOOL,
                    },
                [PYTYPE_STRING] =
                    {
                        [PYTYPE_STRING] = TYPE_INFO_BOOL,
                    },
            },
        [OPERATOR_GREATER_EQUAL] =
            {
                [PYTYPE_INT] =
                    {
                        [PYTYPE_INT] = TYPE_INFO_BOOL,
                        [PYTYPE_FLOAT] = TYPE_INFO_BOOL,
                    },
                [PYTYPE_FLOAT] =
                    {
                        [PYTYPE_INT] = TYPE_INFO_BOOL,
                        [PYTYPE_FLOAT] = TYPE_INFO_BOOL,
                    },
                [PYTYPE_STRING] =
                    {
                        [PYTYPE_STRING] = TYPE_INFO_BOOL,
                    },
            },
        [OPERATOR_LESS_EQUAL] =
            {
                [PYTYPE_INT] =
                    {
                        [PYTYPE_INT] = TYPE_INFO_BOOL,
                        [PYTYPE_FLOAT] = TYPE_INFO_BOOL,
                    },
                [PYTYPE_FLOAT] =
                    {
                        [PYTYPE_INT] = TYPE_INFO_BOOL,
                        [PYTYPE_FLOAT] = TYPE_INFO_BOOL,
                    },
                [PYTYPE_STRING] =
                    {
                        [PYTYPE_STRING] = TYPE_INFO_BOOL,
                    },
            },
        [OPERATOR_BITWISE_AND] =
            {
                [PYTYPE_INT] =
                    {
                        [PYTYPE_INT] = TYPE_INFO_INT,
                    },
            },
        [OPERATOR_BITWISE_OR] =
            {
                [PYTYPE_INT] =
                    {
                        [PYTYPE_INT] = TYPE_INFO_INT,
                    },
            },
        [OPERATOR_BITWISE_XOR] =
            {
                [PYTYPE_INT] =
                    {
                        [PYTYPE_INT] = TYPE_INFO_INT,
                    },
            },
        [OPERATOR_LSHIFT] =
            {
                [PYTYPE_INT] =
                    {
                        [PYTYPE_INT] = TYPE_INFO_INT,
                    },
            },
        [OPERATOR_RSHIFT] =
            {
                [PYTYPE_INT] =
                    {
                        [PYTYPE_INT] = TYPE_INFO_INT,
                    },
            },
};

static TypeInfo
resolve_membership(TypeInfo left, TypeInfo right)
{
    switch (right.type) {
        case PYTYPE_STRING:
            if (left.type == PYTYPE_STRING) return (TypeInfo){.type = PYTYPE_BOOL};
            return (TypeInfo){.type = PYTYPE_UNTYPED};
        case PYTYPE_LIST:
            if (compare_types(left, right.inner->types[0]))
                return (TypeInfo){.type = PYTYPE_BOOL};
            return (TypeInfo){.type = PYTYPE_UNTYPED};
        case PYTYPE_DICT:
            if (compare_types(left, right.inner->types[0]))
                return (TypeInfo){.type = PYTYPE_BOOL};
            return (TypeInfo){.type = PYTYPE_UNTYPED};
        default:
            return (TypeInfo){.type = PYTYPE_UNTYPED};
    }
}

static TypeInfo
resolve_identity(TypeInfo left, TypeInfo right)
{
    switch (left.type) {
        case PYTYPE_LIST:
            if (compare_types(left, right)) return (TypeInfo){.type = PYTYPE_BOOL};
            return (TypeInfo){.type = PYTYPE_UNTYPED};
        case PYTYPE_DICT:
            if (compare_types(left, right)) return (TypeInfo){.type = PYTYPE_BOOL};
            return (TypeInfo){.type = PYTYPE_UNTYPED};
        case PYTYPE_OBJECT:
            if (compare_types(left, right)) return (TypeInfo){.type = PYTYPE_BOOL};
            return (TypeInfo){.type = PYTYPE_UNTYPED};
        case PYTYPE_BOOL:
            if (compare_types(left, right)) return (TypeInfo){.type = PYTYPE_BOOL};
            return (TypeInfo){.type = PYTYPE_UNTYPED};
        default:
            return (TypeInfo){.type = PYTYPE_UNTYPED};
    }
}

static TypeInfo
resolve_negative(TypeInfo right)
{
    if (!is_number(right)) return (TypeInfo){.type = PYTYPE_UNTYPED};
    return right;
}

static TypeInfo
resolve_bitwise_not(TypeInfo right)
{
    if (right.type != PYTYPE_INT) return (TypeInfo){.type = PYTYPE_UNTYPED};
    return right;
}

static TypeInfo
resolve_get_item(TypeInfo left, TypeInfo right)
{
    switch (left.type) {
        case PYTYPE_LIST:
            if (right.type == PYTYPE_SLICE)
                return left;
            else if (right.type == PYTYPE_INT)
                return left.inner->types[0];
            else
                return (TypeInfo){.type = PYTYPE_UNTYPED};
        case PYTYPE_DICT:
            if (!compare_types(left.inner->types[0], right))
                return (TypeInfo){.type = PYTYPE_UNTYPED};
            else
                return left.inner->types[1];
        default:
            UNIMPLEMENTED("getitem type resolution not implemented");
    }
}

static TypeInfo
resolve_list_ops(TypeInfo list, TypeInfo other, Operator op)
{
    switch (op) {
        case OPERATOR_PLUS:
            if (!compare_types(list, other)) goto error;
            return list;
        case OPERATOR_MULT:
            if (other.type != PYTYPE_INT) goto error;
            return list;
        case OPERATOR_EQUAL:
            if (!compare_types(list, other)) goto error;
            return (TypeInfo)TYPE_INFO_BOOL;
        case OPERATOR_NOT_EQUAL:
            if (!compare_types(list, other)) goto error;
            return (TypeInfo)TYPE_INFO_BOOL;
        default:
            goto error;
    }
error:
    return (TypeInfo)TYPE_INFO_UNTYPED;
}

static TypeInfo
resolve_dict_ops(TypeInfo dict, TypeInfo other, Operator op)
{
    (void)dict;
    (void)other;
    (void)op;
    UNIMPLEMENTED("dict operation type resolution unimplemented");
}

static TypeInfo
resolve_tuple_ops(TypeInfo tuple, TypeInfo other, Operator op)
{
    (void)tuple;
    (void)other;
    (void)op;
    UNIMPLEMENTED("tuple operation type resolution unimplemented");
}

static TypeInfo
resolve_object_ops(TypeInfo object, TypeInfo other, Operator op)
{
    (void)object;
    (void)other;
    (void)op;
    UNIMPLEMENTED("object operation type resolution unimplemented");
}

TypeInfo
resolve_operation_type(TypeInfo left, TypeInfo right, Operator op)
{
    if (left.type == PYTYPE_UNTYPED || right.type == PYTYPE_UNTYPED)
        return (TypeInfo)TYPE_INFO_UNTYPED;
    if (SPECIAL_OPERATOR_RULES[op]) goto special_ops;
    if (!TYPE_USES_SPECIAL_RESOLUTION_RULES[left.type] &&
        !TYPE_USES_SPECIAL_RESOLUTION_RULES[right.type]) {
        return OPERATION_TYPE_RESOLUTION_TABLE[op][left.type][right.type];
    }
    else {
        switch (left.type) {
            case PYTYPE_LIST:
                return resolve_list_ops(left, right, op);
            case PYTYPE_DICT:
                return resolve_dict_ops(left, right, op);
            case PYTYPE_TUPLE:
                return resolve_tuple_ops(left, right, op);
            case PYTYPE_OBJECT:
                return resolve_object_ops(left, right, op);
            default:
                break;
        }
        switch (right.type) {
            case PYTYPE_LIST:
                return resolve_list_ops(right, left, op);
            case PYTYPE_DICT:
                return resolve_dict_ops(right, left, op);
            case PYTYPE_TUPLE:
                return resolve_tuple_ops(right, left, op);
            case PYTYPE_OBJECT:
                return resolve_object_ops(right, left, op);
            default:
                UNREACHABLE();
        }
    }
special_ops:
    switch (op) {
        case OPERATOR_CONDITIONAL_IF:
            return left;
        case OPERATOR_CONDITIONAL_ELSE:
            return right;
        case OPERATOR_IN:
            return resolve_membership(left, right);
        case OPERATOR_IS:
            return resolve_identity(left, right);
        case OPERATOR_NEGATIVE:
            return resolve_negative(right);
        case OPERATOR_BITWISE_NOT:
            return resolve_bitwise_not(right);
        case OPERATOR_LOGICAL_AND:
            return (TypeInfo){.type = PYTYPE_BOOL};
        case OPERATOR_LOGICAL_OR:
            return (TypeInfo){.type = PYTYPE_BOOL};
        case OPERATOR_LOGICAL_NOT:
            return (TypeInfo){.type = PYTYPE_BOOL};
        case OPERATOR_GET_ITEM:
            return resolve_get_item(left, right);
        default:
            // assignment ops aren't handled here and should never be passed in
            // to begin with as they are not part of expressions
            break;
    }
    UNREACHABLE();
}

static TypeInfo
resolve_from_scopes(TypeChecker* tc, char* identifier)
{
    Symbol* sym;
    if (tc->locals) {
        sym = symbol_hm_get(&tc->locals->hm, identifier);
        if (sym) return sym->variable->type;
    }
    sym = symbol_hm_get(&tc->globals->hm, identifier);
    switch (sym->kind) {
        case SYM_VARIABLE:
            return sym->variable->type;
        case SYM_SEMI_SCOPED:
            return sym->semi_scoped->type;
        case SYM_CLASS:
            UNIMPLEMENTED("resolve class type from scope");
        case SYM_FUNCTION:
            UNIMPLEMENTED("resolve function type from scope");
    }
    return (TypeInfo){.type = PYTYPE_UNTYPED};
}

TypeInfo
resolve_operand_type(TypeChecker* tc, Operand operand)
{
    switch (operand.kind) {
        case OPERAND_ENCLOSURE_LITERAL:
            UNIMPLEMENTED("enclosure type checking unimplemented");
            break;
        case OPERAND_COMPREHENSION:
            UNIMPLEMENTED("comprehension type checking unimplemented");
            break;
        case OPERAND_TOKEN: {
            if (operand.token.type == TOK_IDENTIFIER) {
                // TODO: should investigate why this isn't a keyword
                if (strcmp(operand.token.value, "None") == 0)
                    return (TypeInfo){.type = PYTYPE_NONE};
                return resolve_from_scopes(tc, operand.token.value);
            }
            return resolve_literal_type(operand.token);
        }
        default:
            // the rest of the operands aren't resolved outside of the scope
            // of an operation
            UNREACHABLE();
    }
    UNREACHABLE();
}
