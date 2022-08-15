CFLAGS = -Wall -Wextra -std=c11

clean:
	-rm debug_lexer

debug_lexer: clean
	gcc $(CFLAGS) -o debug_lexer src/debug_lexer.c src/keywords.c src/lexer.c src/operators.c
	./debug_lexer test/debug_lexer_example.py
