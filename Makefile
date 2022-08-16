CFLAGS = -Wall -Wextra -std=c11
COMPILER_SOURCES = src/keywords.c src/lexer.c src/operators.c src/tokens.c

clean:
	-rm debug_tokens
	-rm debug_instructions

debug: debug_tokens debug_instructions

debug_tokens: clean
	gcc $(CFLAGS) -o debug_tokens src/debug_tokenization.c src/debug_common.c $(COMPILER_SOURCES)
	./debug_tokens test/debug_tokenization.py

debug_instructions: clean
	gcc $(CFLAGS) -o debug_instructions src/debug_instructions.c src/debug_common.c $(COMPILER_SOURCES)
	./debug_instructions test/instruction_parsing/for_loop.py
