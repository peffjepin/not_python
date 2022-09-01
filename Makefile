CC = cc
CFLAGS = -Wall -Wextra -Wpedantic -std=c11
DEBUG_CFLAGS = $(CFLAGS) -g -DDEBUG=1
COMPILER_SOURCES = $(wildcard src/*.c)
DEBUG_SOURCES = test/debug_common.c

clean:
	-rm debug_tokens
	-rm debug_statements
	-rm test_symbol_hashmap
	-rm -rf codegen
	-rm -rf backup

debug: debug_tokens debug_statements

debug_tokens: $(DEBUG_SOURCES) $(COMPILER_SOURCES) test/debug_tokenization.c
	$(CC) $(DEBUG_CFLAGS) -o $@ $^

debug_statements: $(DEBUG_SOURCES) $(COMPILER_SOURCES) test/debug_statements.c
	$(CC) $(DEBUG_CFLAGS) -o $@ $^

test_symbol_hashmap: $(COMPILER_SOURCES) test/test_symbol_hashmap.c
	$(CC) $(DEBUG_CFLAGS) -o $@ $^

run_test_symbol_hashmap: test_symbol_hashmap
	./$^

test: test_tokens test_statements run_test_symbol_hashmap

test_update: test_tokens_interactive test_statements_interactive

test_tokens: debug_tokens
	./scripts/test.py ./debug_tokens ./test/samples/tokenization

test_tokens_interactive: debug_tokens
	./scripts/test.py ./debug_tokens ./test/samples/tokenization -i

test_statements: debug_statements
	./scripts/test.py ./debug_statements ./test/samples/statements

test_statements_interactive: debug_statements
	./scripts/test.py ./debug_statements ./test/samples/statements -i

regenerate:
	-mkdir backup
	-mkdir codegen
	./scripts/codegen.py --out-dir=codegen --module-name=generated
	-cp src/generated.h backup
	-cp src/generated.c backup
	clang-format ./codegen/generated.h > src/generated.h
	clang-format ./codegen/generated.c > src/generated.c
	rm -rf codegen
