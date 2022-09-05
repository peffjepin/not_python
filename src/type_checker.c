#include "type_checker.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
            return outer_equal && (strcmp(type1.class_name, type2.class_name) == 0);
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

static TypeInfo
resolve_plus(TypeInfo left, TypeInfo right)
{
    switch (left.type) {
        case PYTYPE_INT:
            switch (right.type) {
                case PYTYPE_INT:
                    return (TypeInfo){.type = PYTYPE_INT};
                case PYTYPE_FLOAT:
                    return (TypeInfo){.type = PYTYPE_FLOAT};
                default:
                    return (TypeInfo){.type = PYTYPE_UNTYPED};
            }
            break;
        case PYTYPE_FLOAT:
            if (is_number(right)) return left;
            return (TypeInfo){.type = PYTYPE_UNTYPED};
        case PYTYPE_STRING:
            if (right.type == PYTYPE_STRING)
                return (TypeInfo){.type = PYTYPE_STRING};
            else
                return (TypeInfo){.type = PYTYPE_UNTYPED};
        case PYTYPE_LIST:
            if (compare_types(left, right)) return left;
            return (TypeInfo){.type = PYTYPE_UNTYPED};
        default:
            return (TypeInfo){.type = PYTYPE_UNTYPED};
    }
}

static TypeInfo
resolve_minus(TypeInfo left, TypeInfo right)
{
    switch (left.type) {
        case PYTYPE_INT:
            switch (right.type) {
                case PYTYPE_INT:
                    return (TypeInfo){.type = PYTYPE_INT};
                case PYTYPE_FLOAT:
                    return (TypeInfo){.type = PYTYPE_FLOAT};
                default:
                    return (TypeInfo){.type = PYTYPE_UNTYPED};
            }
        case PYTYPE_FLOAT:
            if (is_number(right)) return left;
            return (TypeInfo){.type = PYTYPE_UNTYPED};
        default:
            return (TypeInfo){.type = PYTYPE_UNTYPED};
    }
}

static TypeInfo
resolve_multiply(TypeInfo left, TypeInfo right)
{
    switch (left.type) {
        case PYTYPE_INT:
            switch (right.type) {
                case PYTYPE_INT:
                    return (TypeInfo){.type = PYTYPE_INT};
                case PYTYPE_FLOAT:
                    return (TypeInfo){.type = PYTYPE_FLOAT};
                case PYTYPE_STRING:
                    return right;
                case PYTYPE_LIST:
                    return right;
                default:
                    return (TypeInfo){.type = PYTYPE_UNTYPED};
            }
        case PYTYPE_FLOAT:
            if (is_number(right)) return left;
            return (TypeInfo){.type = PYTYPE_UNTYPED};
        case PYTYPE_STRING:
            if (right.type == PYTYPE_INT) return left;
            return (TypeInfo){.type = PYTYPE_UNTYPED};
        case PYTYPE_LIST:
            if (right.type == PYTYPE_INT) return left;
            return (TypeInfo){.type = PYTYPE_UNTYPED};
        default:
            return (TypeInfo){.type = PYTYPE_UNTYPED};
    }
}

static TypeInfo
resolve_divide(TypeInfo left, TypeInfo right)
{
    switch (left.type) {
        case PYTYPE_INT:
            if (is_number(right)) return (TypeInfo){.type = PYTYPE_FLOAT};
            return (TypeInfo){.type = PYTYPE_UNTYPED};
        case PYTYPE_FLOAT:
            if (is_number(right)) return left;
            return (TypeInfo){.type = PYTYPE_UNTYPED};
        default:
            return (TypeInfo){.type = PYTYPE_UNTYPED};
    }
}

static TypeInfo
resolve_modulo(TypeInfo left, TypeInfo right)
{
    switch (left.type) {
        case PYTYPE_INT:
            if (right.type == PYTYPE_INT) return left;
            return (TypeInfo){.type = PYTYPE_UNTYPED};
        case PYTYPE_FLOAT:
            if (right.type == PYTYPE_INT) return left;
            return (TypeInfo){.type = PYTYPE_UNTYPED};
        case PYTYPE_STRING:
            // TODO: format string
            return (TypeInfo){.type = PYTYPE_UNTYPED};
        default:
            return (TypeInfo){.type = PYTYPE_UNTYPED};
    }
}

static TypeInfo
resolve_power(TypeInfo left, TypeInfo right)
{
    if (!(is_number(right) && is_number(left))) return (TypeInfo){.type = PYTYPE_UNTYPED};
    if (left.type == PYTYPE_FLOAT || right.type == PYTYPE_FLOAT)
        return (TypeInfo){.type = PYTYPE_FLOAT};
    return (TypeInfo){.type = PYTYPE_INT};
}

static TypeInfo
resolve_floordiv(TypeInfo left, TypeInfo right)
{
    if (!(is_number(right) && is_number(left))) return (TypeInfo){.type = PYTYPE_UNTYPED};
    // python returns float if a float is involved, im just going to return an int for now
    return (TypeInfo){.type = PYTYPE_INT};
}

static TypeInfo
resolve_equal(TypeInfo left, TypeInfo right)
{
    if (is_number(left) && is_number(right))
        return (TypeInfo){.type = PYTYPE_BOOL};
    else if (compare_types(left, right))
        return (TypeInfo){.type = PYTYPE_BOOL};
    return (TypeInfo){.type = PYTYPE_UNTYPED};
}

static TypeInfo
resolve_gtlt_comparison(TypeInfo left, TypeInfo right)
{
    if (is_number(left) && is_number(right))
        return (TypeInfo){.type = PYTYPE_BOOL};
    else if (left.type == PYTYPE_STRING && right.type == PYTYPE_STRING)
        return (TypeInfo){.type = PYTYPE_BOOL};
    return (TypeInfo){.type = PYTYPE_UNTYPED};
}

static TypeInfo
resolve_bit_manipulations(TypeInfo left, TypeInfo right)
{
    if (left.type == PYTYPE_INT && right.type == PYTYPE_INT) return right;
    return (TypeInfo){.type = PYTYPE_UNTYPED};
}

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

TypeInfo
resolve_operation_type(TypeInfo left, TypeInfo right, Operator op)
{
    if (left.type == PYTYPE_UNTYPED || right.type == PYTYPE_UNTYPED)
        return (TypeInfo){.type = PYTYPE_UNTYPED};
    switch (op) {
        case OPERATOR_PLUS:
            return resolve_plus(left, right);
        case OPERATOR_MINUS:
            return resolve_minus(left, right);
        case OPERATOR_MULT:
            return resolve_multiply(left, right);
        case OPERATOR_DIV:
            return resolve_divide(left, right);
        case OPERATOR_MOD:
            return resolve_modulo(left, right);
        case OPERATOR_POW:
            return resolve_power(left, right);
        case OPERATOR_FLOORDIV:
            return resolve_floordiv(left, right);
        case OPERATOR_EQUAL:
            return resolve_equal(left, right);
        case OPERATOR_NOT_EQUAL:
            return resolve_equal(left, right);
        case OPERATOR_GREATER:
            return resolve_gtlt_comparison(left, right);
        case OPERATOR_LESS:
            return resolve_gtlt_comparison(left, right);
        case OPERATOR_GREATER_EQUAL:
            return resolve_gtlt_comparison(left, right);
        case OPERATOR_LESS_EQUAL:
            return resolve_gtlt_comparison(left, right);
        case OPERATOR_BITWISE_AND:
            return resolve_bit_manipulations(left, right);
        case OPERATOR_BITWISE_OR:
            return resolve_bit_manipulations(left, right);
        case OPERATOR_BITWISE_XOR:
            return resolve_bit_manipulations(left, right);
        case OPERATOR_CONDITIONAL_IF:
            return left;
        case OPERATOR_CONDITIONAL_ELSE:
            return right;
        case OPERATOR_LSHIFT:
            return resolve_bit_manipulations(left, right);
        case OPERATOR_RSHIFT:
            return resolve_bit_manipulations(left, right);
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
    if (sym) return sym->variable->type;
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
