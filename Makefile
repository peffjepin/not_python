CFLAGS = -Wall -Wextra -std=c11
COMPILER_SOURCES = src/keywords.c src/lexer.c src/operators.c src/tokens.c

clean:
	-@rm debug_tokens
	-@rm debug_instructions

build_debug: build_debug_tokens build_debug_instructions

debug: debug_tokens debug_instructions

build_debug_tokens:
	gcc $(CFLAGS) -o debug_tokens src/debug_tokenization.c src/debug_common.c $(COMPILER_SOURCES)

debug_tokens: clean build_debug_tokens
	./debug_tokens test/debug_tokenization.py

debug_instructions: clean build_debug_instructions
	./scripts/debug_instructions.sh ./debug_instructions ./test/instruction_parsing

build_debug_instructions:
	gcc $(CFLAGS) -o debug_instructions src/debug_instructions.c src/debug_common.c $(COMPILER_SOURCES)

debug_statements: clean build_debug_instructions
	./debug_instructions test/debug_statements.py
