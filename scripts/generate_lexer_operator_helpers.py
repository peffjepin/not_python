#!/usr/bin/env python3

OPS = {
    "PLUS": "+",
    "MINUS": "-",
    "MULT": "*",
    "DIV": "/",
    "MOD": "%",
    "POW": "**",
    "FLOORDIV": "//",
    "ASSIGNMENT": "=",
    "PLUS_ASSIGNMENT": "+=",
    "MINUS_ASSIGNMENT": "-=",
    "MULT_ASSIGNMENT": "*=",
    "DIV_ASSIGNMENT": "/=",
    "MOD_ASSIGNMENT": "%=",
    "FLOORDIV_ASSIGNMENT": "//=",
    "POW_ASSIGNMENT": "**=",
    "AND_ASSIGNMENT": "&=",
    "OR_ASSIGNMENT": "|=",
    "XOR_ASSIGNMENT": "^=",
    "RSHIFT_ASSIGNMENT": ">>=",
    "LSHIFT_ASSIGNMENT": "<<=",
    "EQUAL": "==",
    "NOT_EQUAL": "!=",
    "GREATER": ">",
    "LESS": "<",
    "GREATER_EQUAL": ">=",
    "LESS_EQUAL": "<=",
    "BITWISE_AND": "&",
    "BITWISE_OR": "|",
    "BITWISE_XOR": "^",
    "BITWISE_NOT": "~",
    "LSHIFT": "<<",
    "RSHIFT": ">>",
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
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
