#!/usr/bin/env python3

import argparse
import pathlib

ROOT = pathlib.Path(__file__).parent.parent
THIS = pathlib.Path(__file__).absolute().resolve().relative_to(ROOT)
PREAMBLE_LINES = [
    "/*",
    "!"*80,
    f"this code is generated by `{str(THIS)}` and should not be edited manually",
    "!"*80,
    "*/",
]


class CSource:
    def __init__(self, filename, header=False):
        self.filename = filename + (".h" if header else ".c")
        self.header = header
        if header:
            self.lines = PREAMBLE_LINES.copy() + [
                f"#ifndef {filename.upper()}_H",
                f"#define {filename.upper()}_H",
                "",
            ]
        else:
            self.lines = PREAMBLE_LINES.copy()

    def __str__(self):
        if self.header:
            return "\n".join(self.lines + ["", "#endif"])
        else:
            return "\n".join(self.lines)

    def append(self, line):
        self.lines.append(line)

    def append_sep(self, line):
        self.lines.append(line)
        self.lines.append("")

    def extend(self, lines):
        self.lines.extend(lines)

    def extend_sep(self, lines):
        self.lines.extend(lines)
        self.lines.append("")

    def append_enum(self, name, *auto, **valued):
        self.lines += [
            "typedef enum {",
            *[f"    {name.upper()}," for name in auto],
            *[f"    {name.upper()} = {val}," for name, val in valued.items()],
            f"}} {name};",
            "",
        ]

    def append_struct(self, name, **fields):
        self.lines += [
            "typedef struct {",
            *[f"    {type} {field};" for field, type in fields.items()],
            f"}} {name};",
            "",
        ]


class CFunc:
    def __init__(self, type, name, sig, body):
        self.type = type
        self.name = name
        self.body = body
        self.sig = sig

    @property
    def declaration_line(self):
        return f"{self.type} {self.name}({', '.join(self.sig)});"

    @property
    def definition_lines(self):
        return [
            self.declaration_line[:-1],
            "{",
            *[f"    {line}" for line in self.body],
            "}",
        ]


class CArray:
    def __init__(self, type, name, length, *values, **keyed_values):
        self.type = type
        self.name = name
        self.length = length
        self.values = values
        self.keyed_values = keyed_values

    @property
    def extern_declaration_line(self):
        return f"extern {self.type} {self.name}[{self.length}];"

    @property
    def definition_lines(self):
        return [
            f"{self.type} {self.name}[{self.length}] = {{",
            *[f"    {val}," for val in self.values],
            *[
                f"    [{key}] = {val},"
                for key, val in self.keyed_values.items()
            ],
            "};",
        ]


KEYWORDS = (
    "and",
    "as",
    "assert",
    "break",
    "class",
    "continue",
    "def",
    "del",
    "elif",
    "else",
    "except",
    "finally",
    "for",
    "from",
    "global",
    "if",
    "import",
    "in",
    "is",
    "lambda",
    "nonlocal",
    "not",
    "or",
    "pass",
    "raise",
    "return",
    "try",
    "while",
    "with",
    "yield",
)


def kw_hash(kw, redundancy):
    value = 0
    for c in kw:
        value += ord(c)
    return value % (len(KEYWORDS) + redundancy)


def find_good_keyword_hash_redundancy_value():
    redundancy = 0
    while redundancy < 500:
        seen = set()
        try:
            for kw in KEYWORDS:
                value = kw_hash(kw, redundancy)
                assert value not in seen
                # 0 is reserved return value to indicate NOT a keyword
                assert value != 0
                seen.add(value)
        except AssertionError:
            redundancy += 1
        else:
            return redundancy
    raise AssertionError("no good redundancy value found")


kw_hash_function = CFunc(
    name="kw_hash",
    type="size_t",
    sig=["char* kw"],
    body=[
        "size_t hash = 0;",
        r"for (size_t i = 0; kw[i] != '\0'; i++) hash += kw[i];",
        "return hash % KEYWORDS_MAX;",
    ],
)
is_kw_function = CFunc(
    name="is_keyword",
    type="Keyword",
    sig=["char* word"],
    body=[
        "if (!word) return NOT_A_KEYWORD;",
        "size_t hash = kw_hash(word);",
        "const char* kw = kw_hashtable[hash];",
        "if (!kw) return NOT_A_KEYWORD;",
        "if (strcmp(word, kw_hashtable[hash]) == 0) return hash;",
        "return NOT_A_KEYWORD;",
    ],
)
kw_hashtable_array = CArray(
    name="kw_hashtable",
    type="static const char*",
    length="KEYWORDS_MAX",
    **{f"KW_{kw.upper()}": f'"{kw}"' for kw in KEYWORDS},
)
kw_to_cstr_function = CFunc(
    name="kw_to_cstr",
    type="const char*",
    sig=["Keyword kw"],
    body=["return kw_hashtable[kw];"],
)

# REF
# https://docs.python.org/3/reference/expressions.html#operator-precedence
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
    "OPERATOR_CONDITIONAL_IF": {"value": "if", "precedence": 2},
    "OPERATOR_CONDITIONAL_ELSE": {"value": "else", "precedence": 2},
    "OPERATOR_LSHIFT": {"value": "<<", "precedence": 10},
    "OPERATOR_RSHIFT": {"value": ">>", "precedence": 10},
    "OPERATOR_CALL": {"value": "__call__", "precedence": 16},
    "OPERATOR_GET_ITEM": {"value": "__getitem__", "precedence": 16},
    "OPERATOR_GET_ATTR": {"value": "__getattr__", "precedence": 16},
    "OPERATOR_LOGICAL_AND": {"value": "and", "precedence": 4},
    "OPERATOR_LOGICAL_OR": {"value": "or", "precedence": 3},
    "OPERATOR_LOGICAL_NOT": {"value": "not", "precedence": 5},
    "OPERATOR_IN": {"value": "in", "precedence": 6},
    "OPERATOR_IS": {"value": "is", "precedence": 6},
    "OPERATOR_NEGATIVE": {"value": "neg", "precedence": 13},
    "OPERATOR_POSITIVE": {"value": "pos", "precedence": 13},
    "OPERATOR_BITWISE_NOT": {"value": "~", "precedence": 13},
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


precedence_table_array = CArray(
    name="PRECEDENCE_TABLE",
    type="const unsigned int",
    length="OPERATORS_MAX",
    **{op: d["precedence"] for op, d in OPS.items()},
)
op_to_cstr_array = CArray(
    name="OP_TO_CSTR_TABLE",
    type="static const char*",
    length="OPERATORS_MAX",
    **{op: f'"{d["value"]}"' for op, d in OPS.items()},
)
is_assignement_op_array = CArray(
    name="IS_ASSIGNMENT_OP",
    type="const bool",
    length="OPERATORS_MAX",
    **{
        "OPERATOR_ASSIGNMENT": "true",
        "OPERATOR_PLUS_ASSIGNMENT": "true",
        "OPERATOR_MINUS_ASSIGNMENT": "true",
        "OPERATOR_MULT_ASSIGNMENT": "true",
        "OPERATOR_DIV_ASSIGNMENT": "true",
        "OPERATOR_MOD_ASSIGNMENT": "true",
        "OPERATOR_FLOORDIV_ASSIGNMENT": "true",
        "OPERATOR_POW_ASSIGNMENT": "true",
        "OPERATOR_AND_ASSIGNMENT": "true",
        "OPERATOR_OR_ASSIGNMENT": "true",
        "OPERATOR_XOR_ASSIGNMENT": "true",
        "OPERATOR_RSHIFT_ASSIGNMENT": "true",
        "OPERATOR_LSHIFT_ASSIGNMENT": "true",
    },
)
op_from_cstr_function = CFunc(
    name="op_from_cstr",
    type="Operator",
    sig=["char* op"],
    body=[
        "size_t hash = op[0];",
        r"for (size_t i = 0; op[i] != '\0'; i++) hash += op[i];",
        "return (Operator)hash % OPERATORS_MAX;",
    ],
)
op_to_cstr_function = CFunc(
    name="op_to_cstr",
    type="const char*",
    sig=["Operator op"],
    body=["return OP_TO_CSTR_TABLE[op];"],
)


def main(outdir, module):
    kw_redundancy = find_good_keyword_hash_redundancy_value()
    kw_max = len(KEYWORDS) + kw_redundancy

    op_redundancy = find_good_op_hash_redundancy_value()
    op_max = len(OPS) + op_redundancy
    max_precedence = max(d["precedence"] for d in OPS.values())

    header = CSource(module, header=True)
    source = CSource(module)

    header.append("#include <stdbool.h>")
    header.append_sep("#include <stddef.h>")
    header.append_enum(
        "Keyword",
        **{"NOT_A_KEYWORD": 0, **{f"kw_{kw}": kw_hash(kw, kw_redundancy) for kw in KEYWORDS}},
    )
    header.append(f"#define KEYWORDS_MAX {kw_max}")
    header.append(kw_hash_function.declaration_line)
    header.append(is_kw_function.declaration_line)
    header.append_sep(kw_to_cstr_function.declaration_line)
    header.append_enum(
        "Operator", **{op: op_hash(d["value"], op_redundancy) for op, d in OPS.items()}
    )
    header.append(f"#define OPERATORS_MAX {op_max}")
    header.append(f"#define MAX_PRECEDENCE {max_precedence}")
    header.append(precedence_table_array.extern_declaration_line)
    header.append(is_assignement_op_array.extern_declaration_line)
    header.append(op_from_cstr_function.declaration_line)
    header.append(op_to_cstr_function.declaration_line)

    source.append_sep(f'#include "{header.filename}"')
    source.append_sep("#include <string.h>")
    source.extend_sep(kw_hashtable_array.definition_lines)
    source.extend_sep(kw_hash_function.definition_lines)
    source.extend_sep(is_kw_function.definition_lines)
    source.extend_sep(kw_to_cstr_function.definition_lines)
    source.extend_sep(precedence_table_array.definition_lines)
    source.extend_sep(is_assignement_op_array.definition_lines)
    source.extend_sep(op_to_cstr_array.definition_lines)
    source.extend_sep(op_from_cstr_function.definition_lines)
    source.extend_sep(op_to_cstr_function.definition_lines)

    with open(outdir / header.filename, "w") as f:
        f.write(str(header))
    with open(outdir / source.filename, "w") as f:
        f.write(str(source))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-o",
        "--out-dir",
        help="the directory to save the generated files to",
        default=".",
    )
    parser.add_argument(
        "-m",
        "--module-name",
        help="the name for the generated module",
        default="generated",
    )
    args = parser.parse_args()

    outdir = pathlib.Path(args.out_dir)
    module = args.module_name

    raise SystemExit(main(outdir, module))
