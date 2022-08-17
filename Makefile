CC = cc
CFLAGS = -Wall -Wextra -Wpedantic -std=c11
COMPILER_SOURCES = src/lexer_types.c src/lexer.c

clean:
	-@rm debug_tokens
	-@rm debug_instructions

build_debug: build_debug_tokens build_debug_instructions

debug: debug_tokens debug_instructions

build_debug_tokens:
	$(CC) $(CFLAGS) -o debug_tokens test/debug_tokenization.c test/debug_common.c $(COMPILER_SOURCES)

build_debug_instructions:
	$(CC) $(CFLAGS) -o debug_instructions test/debug_instructions.c test/debug_common.c $(COMPILER_SOURCES)

debug_tokens: clean build_debug_tokens
	./scripts/run_program_across_samples.sh ./debug_tokens ./test/samples/tokenization

debug_instructions: clean build_debug_instructions
	./scripts/run_program_across_samples.sh ./debug_instructions ./test/samples/instructions
