CC = cc
CFLAGS = -Wall -Wextra -Wpedantic -std=c11
COMPILER_SOURCES = $(wildcard src/*.c)
DEBUG_SOURCES = test/debug_common.c

clean:
	-@rm debug_tokens
	-@rm debug_statements

debug: debug_tokens debug_statements

debug_tokens: $(DEBUG_SOURCES) $(COMPILER_SOURCES) test/debug_tokenization.c
	$(CC) -g $(CFLAGS) -DDEBUG=1 -o $@ $^ 

debug_statements: $(DEBUG_SOURCES) $(COMPILER_SOURCES) test/debug_statements.c
	$(CC) -g $(CFLAGS) -DDEBUG=1 -o $@ $^

test: test_tokens test_statements

test_update: test_tokens_interactive test_statements_interactive

test_tokens: debug_tokens
	./scripts/test.py ./debug_tokens ./test/samples/tokenization

test_tokens_interactive: debug_tokens
	./scripts/test.py ./debug_tokens ./test/samples/tokenization -i

test_statements: debug_statements
	./scripts/test.py ./debug_statements ./test/samples/statements

test_statements_interactive: debug_statements
	./scripts/test.py ./debug_statements ./test/samples/statements -i
