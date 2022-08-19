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
    "OPERATOR_LSHIFT": {"value": "<<", "precedence": 10},
    "OPERATOR_RSHIFT": {"value": ">>", "precedence": 10},
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

    print("typedef enum {")
    for op_name, op_dict in OPS.items():
        print(f"    {op_name} = {op_hash(op_dict['value'], redundancy)},")
    print("} Operator;")
    print("")
    print("Operator op_from_cstr(char* op) {")
    print("    size_t hash = op[0];")
    print(r"    for (size_t i = 0; op[i] != '\0'; i++) hash += op[i];")
    print(f"    return (Operator) hash % {total_length};")
    print("}")
    print("")
    print(f"const bool IS_ASSIGNMENT_OP[{total_length}] = {{")
    for op_name, op_dict in OPS.items():
        if "ASSIGNMENT" not in op_name:
            continue
        print(f"    [{op_name}] = true,")
    print("};")
    print("")
    print(f"const unsigned int PRECENDENCE_TABLE[{total_length}] = {{")
    for op_name, op_dict in OPS.items():
        print(f"    [{op_name}] = {op_dict['precedence']},")
    print("};")
    print("")
    print(f"#define MAX_PRECEDENCE {max_precendence}")
    print("")
    print(f"const char* OP_TO_CSTR_TABLE[{total_length}] = {{")
    for op_name, op_dict in OPS.items():
        print(f"    [{op_name}] = \"{op_dict['value']}\",")
    print("};")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
