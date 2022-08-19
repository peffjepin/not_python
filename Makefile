CC = cc
CFLAGS = -Wall -Wextra -Wpedantic -std=c11
COMPILER_SOURCES = $(wildcard src/*.c)
DEBUG_SOURCES = test/debug_common.c

clean:
	-@rm debug_tokens
	-@rm debug_instructions

debug: debug_tokens debug_instructions

debug_tokens: $(DEBUG_SOURCES) $(COMPILER_SOURCES) test/debug_tokenization.c
	$(CC) $(CFLAGS) -o $@ $^

debug_instructions: $(DEBUG_SOURCES) $(COMPILER_SOURCES) test/debug_instructions.c
	$(CC) $(CFLAGS) -o $@ $^

test: test_tokens test_instructions

test_tokens: debug_tokens
	./scripts/run_program_across_samples.sh ./debug_tokens ./test/samples/tokenization

test_instructions: debug_instructions
	./scripts/run_program_across_samples.sh ./debug_instructions ./test/samples/instructions
