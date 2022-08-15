#!/usr/bin/env python3

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


def main():
    redundancy = find_good_keyword_hash_redundancy_value()
    total_length = len(KEYWORDS) + redundancy
    print("typedef enum {")
    print("    NOT_A_KEYWORD = 0,")
    for kw in KEYWORDS:
        print(f"    KW_{kw.upper()} = {kw_hash(kw, redundancy)},")
    print("} Keyword;")
    print("")
    print("size_t kw_hash(char* kw) {")
    print("    size_t hash = 0;")
    print(r"    for (size_t i = 0; kw[i] != '\0'; i++) hash += kw[i];")
    print(f"    return hash % {total_length};")
    print("}")
    print("")
    print(f"const char* kw_hashtable[{total_length}] = {{")
    for kw in KEYWORDS:
        print(f"    [KW_{kw.upper()}] = \"{kw}\",")
    print("};")
    print("")
    print("Keyword is_keyword(char* word) {")
    print("    if (!word) return NOT_A_KEYWORD;")
    print("    size_t hash = kw_hash(word);")
    print("    const char* kw = kw_hashtable[hash];")
    print("    if (!kw) return NOT_A_KEYWORD;")
    print("    if (strcmp(word, kw_hashtable[hash]) == 0) return hash;")
    print("    return NOT_A_KEYWORD;")
    print("}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
