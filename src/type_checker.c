#include "type_checker.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "diagnostics.h"
#include "generated.h"
#include "lexer_helpers.h"

static TypeInfo
resolve_literal_type(Token token)
{
    switch (token.type) {
        case TOK_STRING:
            return (TypeInfo){.type = NPTYPE_STRING};
        case TOK_NUMBER: {
            for (size_t i = 0;; i++) {
                // TODO: this could be handled in the scanner
                char c = token.value.data[i];
                if (c == '\0') return (TypeInfo){.type = NPTYPE_INT};
                if (c == '.' || c == 'f') return (TypeInfo){.type = NPTYPE_FLOAT};
            }
            return (TypeInfo){.type = NPTYPE_INT};
        }
        case TOK_KEYWORD:
            if (token.kw == KW_TRUE || token.kw == KW_FALSE)
                return (TypeInfo){.type = NPTYPE_BOOL};
            break;
        default:
            break;
    }
    return (TypeInfo){.type = NPTYPE_UNTYPED};
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

static bool
compare_signatures(Signature sig1, Signature sig2)
{
    if (sig1.params_count != sig2.params_count) return false;
    if (!compare_types(sig1.return_type, sig2.return_type)) return false;
    for (size_t i = 0; i < sig1.params_count; i++) {
        if (!compare_types(sig1.types[i], sig2.types[i])) return false;
    }
    return true;
}

bool
compare_types(TypeInfo type1, TypeInfo type2)
{
    bool outer_equal = type1.type == type2.type;
    switch (type1.type) {
        case NPTYPE_LIST:
            return outer_equal && compare_inner_type(type1.inner, type2.inner);
        case NPTYPE_TUPLE:
            return outer_equal && compare_inner_type(type1.inner, type2.inner);
        case NPTYPE_DICT:
            return outer_equal && compare_inner_type(type1.inner, type2.inner);
        case NPTYPE_OBJECT:
            return outer_equal && (SOURCESTRING_EQ(type1.cls->name, type2.cls->name));
        case NPTYPE_FUNCTION:
            return outer_equal && compare_signatures(*type1.sig, *type2.sig);
        default:
            return outer_equal;
    }
    UNREACHABLE();
}

static bool
is_number(TypeInfo type)
{
    return (type.type == NPTYPE_INT || type.type == NPTYPE_FLOAT);
}

#define TYPE_INFO_UNTYPED                                                                \
    {                                                                                    \
        .type = NPTYPE_UNTYPED                                                           \
    }
#define TYPE_INFO_FLOAT                                                                  \
    {                                                                                    \
        .type = NPTYPE_FLOAT                                                             \
    }
#define TYPE_INFO_INT                                                                    \
    {                                                                                    \
        .type = NPTYPE_INT                                                               \
    }
#define TYPE_INFO_NONE                                                                   \
    {                                                                                    \
        .type = NPTYPE_NONE                                                              \
    }
#define TYPE_INFO_STRING                                                                 \
    {                                                                                    \
        .type = NPTYPE_STRING                                                            \
    }
#define TYPE_INFO_LIST                                                                   \
    {                                                                                    \
        .type = NPTYPE_LIST                                                              \
    }
#define TYPE_INFO_DICT                                                                   \
    {                                                                                    \
        .type = NPTYPE_DICT                                                              \
    }
#define TYPE_INFO_TUPLE                                                                  \
    {                                                                                    \
        .type = NPTYPE_TUPLE                                                             \
    }
#define TYPE_INFO_OBJECT                                                                 \
    {                                                                                    \
        .type = NPTYPE_OBJECT                                                            \
    }
#define TYPE_INFO_BOOL                                                                   \
    {                                                                                    \
        .type = NPTYPE_BOOL                                                              \
    }
#define TYPE_INFO_SLICE                                                                  \
    {                                                                                    \
        .type = NPTYPE_SLICE                                                             \
    }

static const bool TYPE_USES_SPECIAL_RESOLUTION_RULES[NPTYPE_COUNT] = {
    [NPTYPE_LIST] = true,
    [NPTYPE_DICT] = true,
    [NPTYPE_OBJECT] = true,
    [NPTYPE_TUPLE] = true,
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

// TODO: this table is ~870k (136 * 20 * 20 * 16) at the time of writing and aught to be
//       replaced with something more efficient in the future
//       this alone would reduce the current size of the npc executable by > 50%
static const TypeInfo
    OPERATION_TYPE_RESOLUTION_TABLE[OPERATORS_MAX][NPTYPE_COUNT][NPTYPE_COUNT] = {
        [OPERATOR_PLUS] =
            {
                [NPTYPE_INT] =
                    {
                        [NPTYPE_INT] = TYPE_INFO_INT,
                        [NPTYPE_FLOAT] = TYPE_INFO_FLOAT,
                    },
                [NPTYPE_FLOAT] =
                    {
                        [NPTYPE_INT] = TYPE_INFO_FLOAT,
                        [NPTYPE_FLOAT] = TYPE_INFO_FLOAT,
                    },
                [NPTYPE_STRING] =
                    {
                        [NPTYPE_STRING] = TYPE_INFO_STRING,
                    },
            },
        [OPERATOR_MINUS] =
            {
                [NPTYPE_INT] =
                    {
                        [NPTYPE_INT] = TYPE_INFO_INT,
                        [NPTYPE_FLOAT] = TYPE_INFO_FLOAT,
                    },
                [NPTYPE_FLOAT] =
                    {
                        [NPTYPE_INT] = TYPE_INFO_FLOAT,
                        [NPTYPE_FLOAT] = TYPE_INFO_FLOAT,
                    },
            },
        [OPERATOR_MULT] =
            {
                [NPTYPE_INT] =
                    {
                        [NPTYPE_INT] = TYPE_INFO_INT,
                        [NPTYPE_FLOAT] = TYPE_INFO_FLOAT,
                        [NPTYPE_STRING] = TYPE_INFO_STRING,
                    },
                [NPTYPE_FLOAT] =
                    {
                        [NPTYPE_INT] = TYPE_INFO_FLOAT,
                        [NPTYPE_FLOAT] = TYPE_INFO_FLOAT,
                    },
                [NPTYPE_STRING] =
                    {
                        [NPTYPE_INT] = TYPE_INFO_STRING,
                    },
            },
        [OPERATOR_DIV] =
            {
                [NPTYPE_INT] =
                    {
                        [NPTYPE_FLOAT] = TYPE_INFO_FLOAT,
                        [NPTYPE_INT] = TYPE_INFO_FLOAT,
                    },
                [NPTYPE_FLOAT] =
                    {
                        [NPTYPE_FLOAT] = TYPE_INFO_FLOAT,
                        [NPTYPE_INT] = TYPE_INFO_FLOAT,
                    },
            },
        [OPERATOR_MOD] =
            {
                [NPTYPE_INT] =
                    {
                        [NPTYPE_INT] = TYPE_INFO_INT,
                        [NPTYPE_FLOAT] = TYPE_INFO_FLOAT,
                    },
                [NPTYPE_FLOAT] =
                    {
                        [NPTYPE_INT] = TYPE_INFO_FLOAT,
                        [NPTYPE_FLOAT] = TYPE_INFO_FLOAT,
                    },
            },
        [OPERATOR_POW] =
            {
                [NPTYPE_INT] =
                    {
                        [NPTYPE_INT] = TYPE_INFO_INT,
                        [NPTYPE_FLOAT] = TYPE_INFO_FLOAT,
                    },
                [NPTYPE_FLOAT] =
                    {
                        [NPTYPE_INT] = TYPE_INFO_FLOAT,
                        [NPTYPE_FLOAT] = TYPE_INFO_FLOAT,
                    },
            },
        [OPERATOR_FLOORDIV] =
            {
                [NPTYPE_INT] =
                    {
                        [NPTYPE_INT] = TYPE_INFO_INT,
                        [NPTYPE_FLOAT] = TYPE_INFO_FLOAT,
                    },
                [NPTYPE_FLOAT] =
                    {
                        [NPTYPE_INT] = TYPE_INFO_FLOAT,
                        [NPTYPE_FLOAT] = TYPE_INFO_FLOAT,
                    },
            },
        [OPERATOR_EQUAL] =
            {
                [NPTYPE_INT] =
                    {
                        [NPTYPE_INT] = TYPE_INFO_BOOL,
                        [NPTYPE_FLOAT] = TYPE_INFO_BOOL,
                        [NPTYPE_BOOL] = TYPE_INFO_BOOL,
                    },
                [NPTYPE_FLOAT] =
                    {
                        [NPTYPE_INT] = TYPE_INFO_BOOL,
                        [NPTYPE_FLOAT] = TYPE_INFO_BOOL,
                        [NPTYPE_BOOL] = TYPE_INFO_BOOL,
                    },
                [NPTYPE_BOOL] =
                    {
                        [NPTYPE_INT] = TYPE_INFO_BOOL,
                        [NPTYPE_FLOAT] = TYPE_INFO_BOOL,
                        [NPTYPE_BOOL] = TYPE_INFO_BOOL,
                    },
                [NPTYPE_STRING] =
                    {
                        [NPTYPE_STRING] = TYPE_INFO_BOOL,
                    },
            },
        [OPERATOR_NOT_EQUAL] =
            {
                [NPTYPE_INT] =
                    {
                        [NPTYPE_INT] = TYPE_INFO_BOOL,
                        [NPTYPE_FLOAT] = TYPE_INFO_BOOL,
                        [NPTYPE_BOOL] = TYPE_INFO_BOOL,
                    },
                [NPTYPE_FLOAT] =
                    {
                        [NPTYPE_INT] = TYPE_INFO_BOOL,
                        [NPTYPE_FLOAT] = TYPE_INFO_BOOL,
                        [NPTYPE_BOOL] = TYPE_INFO_BOOL,
                    },
                [NPTYPE_BOOL] =
                    {
                        [NPTYPE_INT] = TYPE_INFO_BOOL,
                        [NPTYPE_FLOAT] = TYPE_INFO_BOOL,
                        [NPTYPE_BOOL] = TYPE_INFO_BOOL,
                    },
                [NPTYPE_STRING] =
                    {
                        [NPTYPE_STRING] = TYPE_INFO_BOOL,
                    },
            },
        [OPERATOR_GREATER] =
            {
                [NPTYPE_INT] =
                    {
                        [NPTYPE_INT] = TYPE_INFO_BOOL,
                        [NPTYPE_FLOAT] = TYPE_INFO_BOOL,
                    },
                [NPTYPE_FLOAT] =
                    {
                        [NPTYPE_INT] = TYPE_INFO_BOOL,
                        [NPTYPE_FLOAT] = TYPE_INFO_BOOL,
                    },
                [NPTYPE_STRING] =
                    {
                        [NPTYPE_STRING] = TYPE_INFO_BOOL,
                    },
            },
        [OPERATOR_LESS] =
            {
                [NPTYPE_INT] =
                    {
                        [NPTYPE_INT] = TYPE_INFO_BOOL,
                        [NPTYPE_FLOAT] = TYPE_INFO_BOOL,
                    },
                [NPTYPE_FLOAT] =
                    {
                        [NPTYPE_INT] = TYPE_INFO_BOOL,
                        [NPTYPE_FLOAT] = TYPE_INFO_BOOL,
                    },
                [NPTYPE_STRING] =
                    {
                        [NPTYPE_STRING] = TYPE_INFO_BOOL,
                    },
            },
        [OPERATOR_GREATER_EQUAL] =
            {
                [NPTYPE_INT] =
                    {
                        [NPTYPE_INT] = TYPE_INFO_BOOL,
                        [NPTYPE_FLOAT] = TYPE_INFO_BOOL,
                    },
                [NPTYPE_FLOAT] =
                    {
                        [NPTYPE_INT] = TYPE_INFO_BOOL,
                        [NPTYPE_FLOAT] = TYPE_INFO_BOOL,
                    },
                [NPTYPE_STRING] =
                    {
                        [NPTYPE_STRING] = TYPE_INFO_BOOL,
                    },
            },
        [OPERATOR_LESS_EQUAL] =
            {
                [NPTYPE_INT] =
                    {
                        [NPTYPE_INT] = TYPE_INFO_BOOL,
                        [NPTYPE_FLOAT] = TYPE_INFO_BOOL,
                    },
                [NPTYPE_FLOAT] =
                    {
                        [NPTYPE_INT] = TYPE_INFO_BOOL,
                        [NPTYPE_FLOAT] = TYPE_INFO_BOOL,
                    },
                [NPTYPE_STRING] =
                    {
                        [NPTYPE_STRING] = TYPE_INFO_BOOL,
                    },
            },
        [OPERATOR_BITWISE_AND] =
            {
                [NPTYPE_INT] =
                    {
                        [NPTYPE_INT] = TYPE_INFO_INT,
                    },
            },
        [OPERATOR_BITWISE_OR] =
            {
                [NPTYPE_INT] =
                    {
                        [NPTYPE_INT] = TYPE_INFO_INT,
                    },
            },
        [OPERATOR_BITWISE_XOR] =
            {
                [NPTYPE_INT] =
                    {
                        [NPTYPE_INT] = TYPE_INFO_INT,
                    },
            },
        [OPERATOR_LSHIFT] =
            {
                [NPTYPE_INT] =
                    {
                        [NPTYPE_INT] = TYPE_INFO_INT,
                    },
            },
        [OPERATOR_RSHIFT] =
            {
                [NPTYPE_INT] =
                    {
                        [NPTYPE_INT] = TYPE_INFO_INT,
                    },
            },
};

static TypeInfo
resolve_membership(TypeInfo left, TypeInfo right)
{
    switch (right.type) {
        case NPTYPE_STRING:
            if (left.type == NPTYPE_STRING) return (TypeInfo){.type = NPTYPE_BOOL};
            return (TypeInfo){.type = NPTYPE_UNTYPED};
        case NPTYPE_LIST:
            if (compare_types(left, right.inner->types[0]))
                return (TypeInfo){.type = NPTYPE_BOOL};
            return (TypeInfo){.type = NPTYPE_UNTYPED};
        case NPTYPE_DICT:
            if (compare_types(left, right.inner->types[0]))
                return (TypeInfo){.type = NPTYPE_BOOL};
            return (TypeInfo){.type = NPTYPE_UNTYPED};
        default:
            return (TypeInfo){.type = NPTYPE_UNTYPED};
    }
}

static TypeInfo
resolve_identity(TypeInfo left, TypeInfo right)
{
    if (compare_types(left, right)) return (TypeInfo){.type = NPTYPE_BOOL};
    return (TypeInfo){.type = NPTYPE_UNTYPED};
}

static TypeInfo
resolve_negative(TypeInfo right)
{
    if (!is_number(right)) return (TypeInfo){.type = NPTYPE_UNTYPED};
    return right;
}

static TypeInfo
resolve_bitwise_not(TypeInfo right)
{
    if (right.type != NPTYPE_INT) return (TypeInfo){.type = NPTYPE_UNTYPED};
    return right;
}

static TypeInfo
resolve_get_item(TypeInfo left, TypeInfo right)
{
    switch (left.type) {
        case NPTYPE_LIST:
            if (right.type == NPTYPE_SLICE)
                return left;
            else if (right.type == NPTYPE_INT)
                return left.inner->types[0];
            else
                return (TypeInfo){.type = NPTYPE_UNTYPED};
        case NPTYPE_DICT:
            if (!compare_types(left.inner->types[0], right))
                return (TypeInfo){.type = NPTYPE_UNTYPED};
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
            if (other.type != NPTYPE_INT) goto error;
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

static const ObjectModel OP_TO_OM_TABLE[OPERATORS_MAX] = {
    [OPERATOR_PLUS] = OBJECT_MODEL_ADD,
    [OPERATOR_MINUS] = OBJECT_MODEL_SUB,
    [OPERATOR_MULT] = OBJECT_MODEL_MUL,
    [OPERATOR_DIV] = OBJECT_MODEL_TRUEDIV,
    [OPERATOR_MOD] = OBJECT_MODEL_MOD,
    [OPERATOR_POW] = OBJECT_MODEL_POW,
    [OPERATOR_FLOORDIV] = OBJECT_MODEL_FLOORDIV,
    [OPERATOR_EQUAL] = OBJECT_MODEL_EQ,
    [OPERATOR_NOT_EQUAL] = OBJECT_MODEL_NE,
    [OPERATOR_GREATER] = OBJECT_MODEL_GT,
    [OPERATOR_LESS] = OBJECT_MODEL_LT,
    [OPERATOR_GREATER_EQUAL] = OBJECT_MODEL_GE,
    [OPERATOR_LESS_EQUAL] = OBJECT_MODEL_LE,
    [OPERATOR_BITWISE_AND] = OBJECT_MODEL_AND,
    [OPERATOR_BITWISE_OR] = OBJECT_MODEL_OR,
    [OPERATOR_BITWISE_XOR] = OBJECT_MODEL_XOR,
    [OPERATOR_LSHIFT] = OBJECT_MODEL_LSHIFT,
    [OPERATOR_RSHIFT] = OBJECT_MODEL_RSHIFT,
    [OPERATOR_CALL] = OBJECT_MODEL_CALL,
    [OPERATOR_GET_ITEM] = OBJECT_MODEL_GETITEM,
    [OPERATOR_NEGATIVE] = OBJECT_MODEL_NEG,
    [OPERATOR_BITWISE_NOT] = OBJECT_MODEL_INVERT,
};

static const ObjectModel OP_TO_ROM_TABLE[OPERATORS_MAX] = {
    [OPERATOR_PLUS] = OBJECT_MODEL_RADD,
    [OPERATOR_MINUS] = OBJECT_MODEL_RSUB,
    [OPERATOR_MULT] = OBJECT_MODEL_RMUL,
    [OPERATOR_DIV] = OBJECT_MODEL_RTRUEDIV,
    [OPERATOR_MOD] = OBJECT_MODEL_RMOD,
    [OPERATOR_POW] = OBJECT_MODEL_RPOW,
    [OPERATOR_FLOORDIV] = OBJECT_MODEL_RFLOORDIV,
    [OPERATOR_BITWISE_AND] = OBJECT_MODEL_RAND,
    [OPERATOR_BITWISE_OR] = OBJECT_MODEL_ROR,
    [OPERATOR_BITWISE_XOR] = OBJECT_MODEL_RXOR,
    [OPERATOR_LSHIFT] = OBJECT_MODEL_RLSHIFT,
    [OPERATOR_RSHIFT] = OBJECT_MODEL_RRSHIFT,
};

FunctionStatement*
find_object_op_function(
    TypeInfo left, TypeInfo right, Operator op, bool* is_rop, bool* is_unary
)
{
    *is_unary = op == OPERATOR_NEGATIVE || op == OPERATOR_BITWISE_NOT;
    *is_rop = false;

    ObjectModel om = OP_TO_OM_TABLE[op];
    if (om != NOT_IN_OBJECT_MODEL && left.type == NPTYPE_OBJECT) {
        FunctionStatement* fndef = left.cls->object_model_methods[om];
        if (fndef && *is_unary) return fndef;
        if (fndef && compare_types(fndef->sig.types[0], right)) return fndef;
    }

    if (*is_unary) return NULL;

    *is_rop = true;
    om = OP_TO_ROM_TABLE[op];
    if (om != NOT_IN_OBJECT_MODEL && right.type == NPTYPE_OBJECT) {
        FunctionStatement* fndef = right.cls->object_model_methods[om];
        if (fndef && compare_types(fndef->sig.types[0], left)) return fndef;
    }

    return NULL;
}

TypeInfo
resolve_operation_type(TypeInfo left, TypeInfo right, Operator op)
{
    if (SPECIAL_OPERATOR_RULES[op]) goto special_ops;
    if (left.type == NPTYPE_UNTYPED || right.type == NPTYPE_UNTYPED)
        return (TypeInfo)TYPE_INFO_UNTYPED;
    if (!TYPE_USES_SPECIAL_RESOLUTION_RULES[left.type] &&
        !TYPE_USES_SPECIAL_RESOLUTION_RULES[right.type]) {
        return OPERATION_TYPE_RESOLUTION_TABLE[op][left.type][right.type];
    }
    if (left.type == NPTYPE_OBJECT || right.type == NPTYPE_OBJECT)
        assert(0 && "object type resolution should not use this function");
    else {
        switch (left.type) {
            case NPTYPE_LIST:
                return resolve_list_ops(left, right, op);
            case NPTYPE_DICT:
                return resolve_dict_ops(left, right, op);
            case NPTYPE_TUPLE:
                return resolve_tuple_ops(left, right, op);
            default:
                break;
        }
        switch (right.type) {
            case NPTYPE_LIST:
                return resolve_list_ops(right, left, op);
            case NPTYPE_DICT:
                return resolve_dict_ops(right, left, op);
            case NPTYPE_TUPLE:
                return resolve_tuple_ops(right, left, op);
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
            return (TypeInfo){.type = NPTYPE_BOOL};
        case OPERATOR_LOGICAL_OR:
            return (TypeInfo){.type = NPTYPE_BOOL};
        case OPERATOR_LOGICAL_NOT:
            return (TypeInfo){.type = NPTYPE_BOOL};
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
resolve_from_scopes(TypeChecker* tc, SourceString identifier)
{
    Symbol* sym = get_symbol_from_scopes(*tc->stack, identifier);
    if (sym) {
        switch (sym->kind) {
            case SYM_GLOBAL:
                return sym->globalvar->type_info;
            case SYM_VARIABLE:
                return sym->variable->type_info;
            case SYM_CLASS:
                UNIMPLEMENTED("resolve class type from scope");
            case SYM_FUNCTION:
                return (TypeInfo){.type = NPTYPE_FUNCTION, .sig = &sym->func->sig};
            case SYM_MEMBER:
                UNREACHABLE();
        }
    }
    return (TypeInfo){.type = NPTYPE_UNTYPED};
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
                if (strcmp(operand.token.value.data, "None") == 0)
                    return (TypeInfo){.type = NPTYPE_NONE};
                return resolve_from_scopes(tc, operand.token.value);
            }
            return resolve_literal_type(operand.token);
        }
        default:
            // the rest of the operands aren't resolved outside of the scope
            // of an operation
            errorf("unexpected operand kind: %i", operand.kind);
    }
    UNREACHABLE();
}
