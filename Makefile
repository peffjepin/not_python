CFLAGS = -Wall -Wextra -std=c11

clean:
	-rm debug_token

debug_token: clean
	gcc $(CFLAGS) -o debug_token src/debug_tokenization.c src/keywords.c src/lexer.c src/operators.c src/tokens.c
	./debug_token test/debug_tokenization.py
