#!/usr/bin/env python3

OPS = {
    "OPERATOR_PLUS": {"value": "+", "precedence": 11},
    "OPERATOR_MINUS": {"value": "-", "precedence": 11},
    "OPERATOR_MULT": {"value": "*", "precedence": 12},
    "OPERATOR_DIV": {"value": "/", "precedence": 12},
    "OPERATOR_MOD": {"value": "%", "precedence": 12},
    "OPERATOR_POW": {"value": "**", "precedence": 14},
    "OPERATOR_FLOORDIV": {"value": "//", "precedence": 12},
    "OPERATOR_ASSIGNMENT": {"value": "=", "precedence": 0},
    "OPERATOR_PLUS_ASSIGNMENT": {"value": "+=", "precedence": 0},
    "OPERATOR_MINUS_ASSIGNMENT": {"value": "-=", "precedence": 0},
    "OPERATOR_MULT_ASSIGNMENT": {"value": "*=", "precedence": 0},
    "OPERATOR_DIV_ASSIGNMENT": {"value": "/=", "precedence": 0},
    "OPERATOR_MOD_ASSIGNMENT": {"value": "%=", "precedence": 0},
    "OPERATOR_FLOORDIV_ASSIGNMENT": {"value": "//=", "precedence": 0},
    "OPERATOR_POW_ASSIGNMENT": {"value": "**=", "precedence": 0},
    "OPERATOR_AND_ASSIGNMENT": {"value": "&=", "precedence": 0},
    "OPERATOR_OR_ASSIGNMENT": {"value": "|=", "precedence": 0},
    "OPERATOR_XOR_ASSIGNMENT": {"value": "^=", "precedence": 0},
    "OPERATOR_RSHIFT_ASSIGNMENT": {"value": ">>=", "precedence": 0},
    "OPERATOR_LSHIFT_ASSIGNMENT": {"value": "<<=", "precedence": 0},
    "OPERATOR_EQUAL": {"value": "==", "precedence": 6},
    "OPERATOR_NOT_EQUAL": {"value": "!=", "precedence": 6},
    "OPERATOR_GREATER": {"value": ">", "precedence": 6},
    "OPERATOR_LESS": {"value": "<", "precedence": 6},
    "OPERATOR_GREATER_EQUAL": {"value": ">=", "precedence": 6},
    "OPERATOR_LESS_EQUAL": {"value": "<=", "precedence": 6},
    "OPERATOR_BITWISE_AND": {"value": "&", "precedence": 9},
    "OPERATOR_BITWISE_OR": {"value": "|", "precedence": 7},
    "OPERATOR_BITWISE_XOR": {"value": "^", "precedence": 8},
    "OPERATOR_BITWISE_NOT": {"value": "~", "precedence": 0},
    "OPERATOR_CONDITIONAL_IF": {"value": "if", "precedence": 2},
    "OPERATOR_CONDITIONAL_ELSE": {"value": "else", "precedence": 2},
    "OPERATOR_LSHIFT": {"value": "<<", "precedence": 10},
    "OPERATOR_RSHIFT": {"value": ">>", "precedence": 10},
    "OPERATOR_CALL": {"value": "__call__", "precedence": 16},
    "OPERATOR_GET_ITEM": {"value": "__getitem__", "precedence": 16},
    "OPERATOR_GET_ATTR": {"value": "__getattr__", "precedence": 16},
}


def op_hash(op, redundancy):
    value = ord(op[0])
    for c in op:
        value += ord(c)
    return value % (len(OPS) + redundancy)


def find_good_op_hash_redundancy_value():
    redundancy = 0
    while redundancy < 500:
        seen = set()
        try:
            for dict_ in OPS.values():
                value = op_hash(dict_["value"], redundancy)
                assert value not in seen
                seen.add(value)
        except AssertionError:
            redundancy += 1
        else:
            return redundancy
    raise AssertionError("no good redundancy value found")


def main():
    redundancy = find_good_op_hash_redundancy_value()
    total_length = len(OPS) + redundancy
    max_precendence = 0
    for dict_ in OPS.values():
        if dict_["precedence"] > max_precendence:
            max_precendence = dict_["precedence"]

    print(f"#define OPERATOR_TABLE_MAX {total_length}")
    print("")
    print("typedef enum {")
    for op_name, op_dict in OPS.items():
        print(f"    {op_name} = {op_hash(op_dict['value'], redundancy)},")
    print("} Operator;")
    print("")
    print("Operator op_from_cstr(char* op) {")
    print("    size_t hash = op[0];")
    print(r"    for (size_t i = 0; op[i] != '\0'; i++) hash += op[i];")
    print("    return (Operator) hash % OPERATOR_TABLE_MAX;")
    print("}")
    print("")
    print("const bool IS_ASSIGNMENT_OP[OPERATOR_TABLE_MAX] = {")
    for op_name, op_dict in OPS.items():
        if "ASSIGNMENT" not in op_name:
            continue
        print(f"    [{op_name}] = true,")
    print("};")
    print("")
    print("const unsigned int PRECENDENCE_TABLE[OPERATOR_TABLE_MAX] = {")
    for op_name, op_dict in OPS.items():
        print(f"    [{op_name}] = {op_dict['precedence']},")
    print("};")
    print("")
    print(f"#define MAX_PRECEDENCE {max_precendence}")
    print("")
    print("const char* OP_TO_CSTR_TABLE[OPERATOR_TABLE_MAX] = {")
    for op_name, op_dict in OPS.items():
        print(f"    [{op_name}] = \"{op_dict['value']}\",")
    print("};")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
