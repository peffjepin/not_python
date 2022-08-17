#!/usr/bin/env python3

OPS = {
    "OP_PLUS": "+",
    "OP_MINUS": "-",
    "OP_MULT": "*",
    "OP_DIV": "/",
    "OP_MOD": "%",
    "OP_POW": "**",
    "OP_FLOORDIV": "//",
    "OP_ASSIGNMENT": "=",
    "OP_PLUS_ASSIGNMENT": "+=",
    "OP_MINUS_ASSIGNMENT": "-=",
    "OP_MULT_ASSIGNMENT": "*=",
    "OP_DIV_ASSIGNMENT": "/=",
    "OP_MOD_ASSIGNMENT": "%=",
    "OP_FLOORDIV_ASSIGNMENT": "//=",
    "OP_POW_ASSIGNMENT": "**=",
    "OP_AND_ASSIGNMENT": "&=",
    "OP_OR_ASSIGNMENT": "|=",
    "OP_XOR_ASSIGNMENT": "^=",
    "OP_RSHIFT_ASSIGNMENT": ">>=",
    "OP_LSHIFT_ASSIGNMENT": "<<=",
    "OP_EQUAL": "==",
    "OP_NOT_EQUAL": "!=",
    "OP_GREATER": ">",
    "OP_LESS": "<",
    "OP_GREATER_EQUAL": ">=",
    "OP_LESS_EQUAL": "<=",
    "OP_BITWISE_AND": "&",
    "OP_BITWISE_OR": "|",
    "OP_BITWISE_XOR": "^",
    "OP_BITWISE_NOT": "~",
    "OP_LSHIFT": "<<",
    "OP_RSHIFT": ">>",
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
        print(f"    [{op_name}] = {op_hash(op_value, redundancy)},")
    print("};")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
