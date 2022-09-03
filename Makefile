CC = cc
CFLAGS = -Wall -Wextra -Wpedantic -std=c11
DEBUG_CFLAGS = $(CFLAGS) -g -DDEBUG=1
SOURCES = $(wildcard src/*.c)
OBJECTS = $(patsubst src/%.c, build/%.o, $(SOURCES))
OBJECTS_DEBUG = $(patsubst src/%.c, build/%_db.o, $(SOURCES))
DEBUG_SOURCES = test/debug_common.c

test: test_tokens test_statements test_lexical_scopes test_c_compiler run_test_symbol_hashmap

test_update: test_tokens_interactive test_statements_interactive test_lexical_scopes_interactive test_c_compiler_interactive

clean:
	-rm -rf build
	-rm debug_tokens
	-rm debug_statements
	-rm debug_lexical_scopes
	-rm debug_c_compiler
	-rm test_symbol_hashmap
	-rm -rf codegen
	-rm -rf backup

regenerate:
	-mkdir backup
	-mkdir codegen
	./scripts/codegen.py --out-dir=codegen --module-name=generated
	-cp src/generated.h backup
	-cp src/generated.c backup
	clang-format ./codegen/generated.h > src/generated.h
	clang-format ./codegen/generated.c > src/generated.c
	rm -rf codegen

build/%.o: src/%.c
	@mkdir -p build
	$(CC) -c $^ -o $@

build/%_db.o: src/%.c
	@mkdir -p build
	$(CC) $(DEBUG_CFLAGS) -c $^ -o $@

debug_tokens: $(DEBUG_SOURCES) $(OBJECTS_DEBUG) test/debug_tokenization.c
	$(CC) $(DEBUG_CFLAGS) -o $@ $^

debug_statements: $(DEBUG_SOURCES) $(OBJECTS_DEBUG) test/debug_statements.c
	$(CC) $(DEBUG_CFLAGS) -o $@ $^

debug_lexical_scopes: $(OBJECTS_DEBUG) $(DEBUG_SOURCES) test/debug_lexical_scopes.c
	$(CC) $(DEBUG_CFLAGS) -o $@ $^

debug_c_compiler: $(OBJECTS_DEBUG) $(DEBUG_SOURCES) test/debug_c_compiler.c
	$(CC) $(DEBUG_CFLAGS) -o $@ $^

test_symbol_hashmap: $(OBJECTS_DEBUG) test/test_symbol_hashmap.c
	$(CC) $(DEBUG_CFLAGS) -o $@ $^

run_test_symbol_hashmap: test_symbol_hashmap
	./$^

test_lexical_scopes: debug_lexical_scopes
	./scripts/test.py ./$^ ./test/samples/scoping

test_lexical_scopes_interactive: debug_lexical_scopes
	./scripts/test.py ./$^ ./test/samples/scoping -i

test_c_compiler: debug_c_compiler
	./scripts/test.py ./$^ ./test/samples/compiler

test_c_compiler_interactive: debug_c_compiler
	./scripts/test.py ./$^ ./test/samples/compiler -i

test_tokens: debug_tokens
	./scripts/test.py ./$^ ./test/samples/tokenization

test_tokens_interactive: debug_tokens
	./scripts/test.py ./$^ ./test/samples/tokenization -i

test_statements: debug_statements
	./scripts/test.py ./$^ ./test/samples/statements

test_statements_interactive: debug_statements
	./scripts/test.py ./$^ ./test/samples/statements -i
