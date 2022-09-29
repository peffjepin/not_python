#include "object_model.h"

#include <string.h>

#define OM_SWITCH_FINISH(rest_of_string, OM)                                             \
    if (strcmp(cstr, rest_of_string "__") == 0) return OM;                               \
    break

static const ObjectModel OP_ASSIGNMENT_TO_OBJECT_MODEL_TABLE[OPERATORS_MAX] = {
    [OPERATOR_PLUS_ASSIGNMENT] = OBJECT_MODEL_IADD,
    [OPERATOR_MINUS_ASSIGNMENT] = OBJECT_MODEL_ISUB,
    [OPERATOR_MULT_ASSIGNMENT] = OBJECT_MODEL_IMUL,
    [OPERATOR_DIV_ASSIGNMENT] = OBJECT_MODEL_ITRUEDIV,
    [OPERATOR_MOD_ASSIGNMENT] = OBJECT_MODEL_IMOD,
    [OPERATOR_FLOORDIV_ASSIGNMENT] = OBJECT_MODEL_IFLOORDIV,
    [OPERATOR_POW_ASSIGNMENT] = OBJECT_MODEL_IPOW,
    [OPERATOR_AND_ASSIGNMENT] = OBJECT_MODEL_IAND,
    [OPERATOR_OR_ASSIGNMENT] = OBJECT_MODEL_IOR,
    [OPERATOR_XOR_ASSIGNMENT] = OBJECT_MODEL_IXOR,
    [OPERATOR_RSHIFT_ASSIGNMENT] = OBJECT_MODEL_IRSHIFT,
    [OPERATOR_LSHIFT_ASSIGNMENT] = OBJECT_MODEL_ILSHIFT,
};

ObjectModel
op_assignment_to_object_model(Operator op_type)
{
    return OP_ASSIGNMENT_TO_OBJECT_MODEL_TABLE[op_type];
}

ObjectModel
source_string_to_object_model(SourceString str)
{
    if (!str.data) return NOT_IN_OBJECT_MODEL;
    if (str.length < 6) return NOT_IN_OBJECT_MODEL;

    char* cstr = str.data;
    if (*cstr++ != '_') return NOT_IN_OBJECT_MODEL;
    if (*cstr++ != '_') return NOT_IN_OBJECT_MODEL;

    switch (*cstr++) {
        case 'a':
            switch (*cstr++) {
                case 'b':
                    OM_SWITCH_FINISH("s", OBJECT_MODEL_ABS);
                case 'd':
                    OM_SWITCH_FINISH("d", OBJECT_MODEL_ADD);
                case 'n':
                    OM_SWITCH_FINISH("d", OBJECT_MODEL_AND);
                default:
                    return NOT_IN_OBJECT_MODEL;
            }
            break;
        case 'b':
            OM_SWITCH_FINISH("ool", OBJECT_MODEL_BOOL);
        case 'c':
            switch (*cstr++) {
                case 'a':
                    OM_SWITCH_FINISH("ll", OBJECT_MODEL_CALL);
                case 'e':
                    OM_SWITCH_FINISH("il", OBJECT_MODEL_CEIL);
                case 'o':
                    OM_SWITCH_FINISH("ntains", OBJECT_MODEL_CONTAINS);
                default:
                    return NOT_IN_OBJECT_MODEL;
            }
            break;
        case 'd':
            switch (*cstr++) {
                case 'e':
                    OM_SWITCH_FINISH("litem", OBJECT_MODEL_DELITEM);
                case 'i':
                    OM_SWITCH_FINISH("vmod", OBJECT_MODEL_DIVMOD);
                default:
                    return NOT_IN_OBJECT_MODEL;
            }
            break;
        case 'e':
            switch (*cstr++) {
                case 'n':
                    OM_SWITCH_FINISH("ter", OBJECT_MODEL_ENTER);
                case 'q':
                    OM_SWITCH_FINISH("", OBJECT_MODEL_EQ);
                case 'x':
                    OM_SWITCH_FINISH("it", OBJECT_MODEL_EXIT);
                default:
                    return NOT_IN_OBJECT_MODEL;
            }
            break;
        case 'f':
            switch (*cstr++) {
                case 'l':
                    if (strcmp("oat__", cstr) == 0) return OBJECT_MODEL_FLOAT;
                    if (strcmp("oor__", cstr) == 0) return OBJECT_MODEL_FLOOR;
                    if (strcmp("oordiv__", cstr) == 0) return OBJECT_MODEL_FLOORDIV;
                    break;
                default:
                    return NOT_IN_OBJECT_MODEL;
            }
            break;
        case 'g':
            switch (*cstr++) {
                case 'e':
                    if (strcmp("__", cstr) == 0) return OBJECT_MODEL_GE;
                    if (strcmp("titem__", cstr) == 0) return OBJECT_MODEL_GETITEM;
                    break;
                case 't':
                    OM_SWITCH_FINISH("", OBJECT_MODEL_GT);
                default:
                    return NOT_IN_OBJECT_MODEL;
            }
            break;
        case 'h':
            OM_SWITCH_FINISH("ash", OBJECT_MODEL_HASH);
        case 'i':
            switch (*cstr++) {
                case 'a':
                    if (strcmp("dd__", cstr) == 0) return OBJECT_MODEL_IADD;
                    if (strcmp("nd__", cstr) == 0) return OBJECT_MODEL_IAND;
                    break;
                case 'f':
                    OM_SWITCH_FINISH("loordiv", OBJECT_MODEL_IFLOORDIV);
                case 'l':
                    OM_SWITCH_FINISH("shift", OBJECT_MODEL_ILSHIFT);
                case 'm':
                    if (strcmp("od__", cstr) == 0) return OBJECT_MODEL_IMOD;
                    if (strcmp("ul__", cstr) == 0) return OBJECT_MODEL_IMUL;
                    break;
                case 'n':
                    if (strcmp("it__", cstr) == 0) return OBJECT_MODEL_INIT;
                    if (strcmp("t__", cstr) == 0) return OBJECT_MODEL_INT;
                    if (strcmp("vert__", cstr) == 0) return OBJECT_MODEL_INVERT;
                    break;
                case 'o':
                    OM_SWITCH_FINISH("r", OBJECT_MODEL_IOR);
                case 'p':
                    OM_SWITCH_FINISH("ow", OBJECT_MODEL_IPOW);
                case 'r':
                    OM_SWITCH_FINISH("shift", OBJECT_MODEL_IRSHIFT);
                case 's':
                    OM_SWITCH_FINISH("ub", OBJECT_MODEL_ISUB);
                case 't':
                    if (strcmp("er__", cstr) == 0) return OBJECT_MODEL_ITER;
                    if (strcmp("ruediv__", cstr) == 0) return OBJECT_MODEL_ITRUEDIV;
                    break;
                case 'x':
                    OM_SWITCH_FINISH("or", OBJECT_MODEL_IXOR);
                default:
                    return NOT_IN_OBJECT_MODEL;
            }
            break;
        case 'l':
            switch (*cstr++) {
                case 'e':
                    if (strcmp("__", cstr) == 0) return OBJECT_MODEL_LE;
                    if (strcmp("n__", cstr) == 0) return OBJECT_MODEL_LEN;
                    break;
                case 's':
                    OM_SWITCH_FINISH("hift", OBJECT_MODEL_LSHIFT);
                case 't':
                    OM_SWITCH_FINISH("", OBJECT_MODEL_LT);
                default:
                    return NOT_IN_OBJECT_MODEL;
            }
            break;
        case 'm':
            switch (*cstr++) {
                case 'o':
                    OM_SWITCH_FINISH("d", OBJECT_MODEL_MOD);
                case 'u':
                    OM_SWITCH_FINISH("l", OBJECT_MODEL_MUL);
                default:
                    return NOT_IN_OBJECT_MODEL;
            }
            break;
        case 'n':
            if (strcmp("e__", cstr) == 0) return OBJECT_MODEL_NE;
            if (strcmp("eg__", cstr) == 0) return OBJECT_MODEL_NEG;
            if (strcmp("ext__", cstr) == 0) return OBJECT_MODEL_NEXT;
            break;
        case 'o':
            OM_SWITCH_FINISH("r", OBJECT_MODEL_OR);
        case 'p':
            OM_SWITCH_FINISH("ow", OBJECT_MODEL_POW);
        case 'r':
            switch (*cstr++) {
                case 'a':
                    if (strcmp("dd__", cstr) == 0) return OBJECT_MODEL_RADD;
                    if (strcmp("nd__", cstr) == 0) return OBJECT_MODEL_RAND;
                    break;
                case 'e':
                    OM_SWITCH_FINISH("pr", OBJECT_MODEL_REPR);
                case 'f':
                    OM_SWITCH_FINISH("loordiv", OBJECT_MODEL_RFLOORDIV);
                case 'l':
                    OM_SWITCH_FINISH("shift", OBJECT_MODEL_RLSHIFT);
                case 'm':
                    if (strcmp("od__", cstr) == 0) return OBJECT_MODEL_RMOD;
                    if (strcmp("ul__", cstr) == 0) return OBJECT_MODEL_RMUL;
                    break;
                case 'o':
                    if (strcmp("r__", cstr) == 0) return OBJECT_MODEL_ROR;
                    if (strcmp("und__", cstr) == 0) return OBJECT_MODEL_ROUND;
                    break;
                case 'p':
                    OM_SWITCH_FINISH("ow", OBJECT_MODEL_RPOW);
                case 'r':
                    OM_SWITCH_FINISH("shift", OBJECT_MODEL_RRSHIFT);
                case 's':
                    if (strcmp("hift__", cstr) == 0) return OBJECT_MODEL_RSHIFT;
                    if (strcmp("ub__", cstr) == 0) return OBJECT_MODEL_RSUB;
                    break;
                case 't':
                    OM_SWITCH_FINISH("ruediv", OBJECT_MODEL_RTRUEDIV);
                case 'x':
                    OM_SWITCH_FINISH("or", OBJECT_MODEL_RXOR);
                default:
                    return NOT_IN_OBJECT_MODEL;
                    break;
            }
            break;
        case 's':
            switch (*cstr++) {
                case 'e':
                    OM_SWITCH_FINISH("titem", OBJECT_MODEL_SETITEM);
                case 't':
                    OM_SWITCH_FINISH("r", OBJECT_MODEL_STR);
                case 'u':
                    OM_SWITCH_FINISH("b", OBJECT_MODEL_SUB);
                default:
                    return NOT_IN_OBJECT_MODEL;
            }
            break;
        case 't':
            if (strcmp("ruediv__", cstr) == 0) return OBJECT_MODEL_TRUEDIV;
            if (strcmp("runc__", cstr) == 0) return OBJECT_MODEL_TRUNC;
            break;
        case 'x':
            OM_SWITCH_FINISH("or", OBJECT_MODEL_XOR);
        default:
            return NOT_IN_OBJECT_MODEL;
    }
    return NOT_IN_OBJECT_MODEL;
}
