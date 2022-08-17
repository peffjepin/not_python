#!/usr/bin/env python3

OPS = {
    "OPERATOR_PLUS": "+",
    "OPERATOR_MINUS": "-",
    "OPERATOR_MULT": "*",
    "OPERATOR_DIV": "/",
    "OPERATOR_MOD": "%",
    "OPERATOR_POW": "**",
    "OPERATOR_FLOORDIV": "//",
    "OPERATOR_ASSIGNMENT": "=",
    "OPERATOR_PLUS_ASSIGNMENT": "+=",
    "OPERATOR_MINUS_ASSIGNMENT": "-=",
    "OPERATOR_MULT_ASSIGNMENT": "*=",
    "OPERATOR_DIV_ASSIGNMENT": "/=",
    "OPERATOR_MOD_ASSIGNMENT": "%=",
    "OPERATOR_FLOORDIV_ASSIGNMENT": "//=",
    "OPERATOR_POW_ASSIGNMENT": "**=",
    "OPERATOR_AND_ASSIGNMENT": "&=",
    "OPERATOR_OR_ASSIGNMENT": "|=",
    "OPERATOR_XOR_ASSIGNMENT": "^=",
    "OPERATOR_RSHIFT_ASSIGNMENT": ">>=",
    "OPERATOR_LSHIFT_ASSIGNMENT": "<<=",
    "OPERATOR_EQUAL": "==",
    "OPERATOR_NOT_EQUAL": "!=",
    "OPERATOR_GREATER": ">",
    "OPERATOR_LESS": "<",
    "OPERATOR_GREATER_EQUAL": ">=",
    "OPERATOR_LESS_EQUAL": "<=",
    "OPERATOR_BITWISE_AND": "&",
    "OPERATOR_BITWISE_OR": "|",
    "OPERATOR_BITWISE_XOR": "^",
    "OPERATOR_BITWISE_NOT": "~",
    "OPERATOR_LSHIFT": "<<",
    "OPERATOR_RSHIFT": ">>",
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
            for op in OPS.values():
                value = op_hash(op, redundancy)
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

    print("typedef enum {")
    for op_name, op_value in OPS.items():
        print(f"    {op_name} = {op_hash(op_value, redundancy)},")
    print("} Operator;")
    print("")
    print("Operator op_from_cstr(char* op) {")
    print("    size_t hash = op[0];")
    print(r"    for (size_t i = 0; op[i] != '\0'; i++) hash += op[i];")
    print(f"    return (Operator) hash % {total_length};")
    print("}")
    print("")
    print(f"const bool IS_ASSIGNMENT_OP[{total_length}] = {{")
    for op_name, op_value in OPS.items():
        if "ASSIGNMENT" not in op_name:
            continue
        print(f"    [{op_name}] = true,")
    print("};")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
