INSTALL_DIR ?= /usr

CC = cc

CFLAGS_COMMON = -Wall -Wextra -Wpedantic -std=c11
DEBUG_CFLAGS = $(CFLAGS_COMMON) -g
RELEASE_CFLAGS = $(CFLAGS_COMMON) -O3

CPPFLAGS_COMMON = -I./src
DEBUG_CPPFLAGS = $(CPPFLAGS_COMMON) -DDEBUG=1 -DINSTALL_DIR=\"$(shell pwd)/build\"
RELEASE_CPPFLAGS = $(CPPFLAGS_COMMON) -DINSTALL_DIR=\"$(INSTALL_DIR)\"

ALL_SOURCES = $(wildcard src/*.c)
SOURCES := $(filter-out $(wildcard src/debug*.c), $(ALL_SOURCES))
OBJECTS = $(patsubst src/%.c, build/%.o, $(SOURCES))
OBJECTS_DEBUG = $(patsubst src/%.c, build/%_db.o, $(ALL_SOURCES))

debug: $(OBJECTS_DEBUG)
	cp src/not_python.h build/not_python.h
	$(CC) $(DEBUG_CPPFLAGS) $(DEBUG_CFLAGS) -o npc $^

release: $(OBJECTS)
	$(CC) $(RELEASE_CPPFLAGS) $(RELEASE_CFLAGS) -o npc $^

clean:
	-rm -rf build
	-rm -rf debug
	-rm -rf codegen
	-rm -rf backup
	-rm -rf npc_build
	-rm test_symbol_hashmap
	-rm npc
	-rm testmain

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
	$(CC) $(RELEASE_CPPFLAGS) $(RELEASE_CFLAGS) -c $^ -o $@

build/%_db.o: src/%.c
	@mkdir -p build
	$(CC) $(DEBUG_CPPFLAGS) $(DEBUG_CFLAGS) -c $^ -o $@

install: build/not_python.o src/not_python.h release
	install -d $(INSTALL_DIR)/lib/
	install -m 644 build/not_python.o $(INSTALL_DIR)/lib/
	install -d $(INSTALL_DIR)/include/
	install -m 644 src/not_python.h $(INSTALL_DIR)/include/
	install -d $(INSTALL_DIR)/bin/
	install -m 777 npc $(INSTALL_DIR)/bin/

uninstall:
	-rm $(INSTALL_DIR)/lib/not_python.o
	-rm $(INSTALL_DIR)/include/not_python.h
	-rm $(INSTALL_DIR)/bin/npc

test: run_test_symbol_hashmap test_tokens test_statements test_lexical_scopes test_c_compiler test_programs 

test_update: test_tokens_interactive test_statements_interactive test_lexical_scopes_interactive test_c_compiler_interactive test_programs_interactive

test_symbol_hashmap: build/hash_db.o build/arena_db.o build/hashmap_db.o test/test_symbol_hashmap.c
	$(CC) $(DEBUG_CFLAGS) -o $@ $^

run_test_symbol_hashmap: test_symbol_hashmap
	./$^
	@echo "PASS"

test_programs: debug
	./scripts/test.py --command="./npc -o testmain --file --run" --directory="test/samples/programs"

test_programs_interactive: debug
	./scripts/test.py --command="./npc -o testmain --file --run" --directory="test/samples/programs" --interactive

test_lexical_scopes: debug
	./scripts/test.py --command="./npc --debug-scopes --file" --directory="test/samples/scoping"

test_lexical_scopes_interactive: debug
	./scripts/test.py --command="./npc --debug-scopes --file" --directory="test/samples/scoping" --interactive

test_c_compiler: debug
	./scripts/test.py --command="./npc --debug-c-compiler --file" --directory="test/samples/compiler"

test_c_compiler_interactive: debug
	./scripts/test.py --command="./npc --debug-c-compiler --file" --directory="test/samples/compiler" --interactive

test_tokens: debug
	./scripts/test.py --command="./npc --debug-tokens --file" --directory="test/samples/tokenization"

test_tokens_interactive: debug
	./scripts/test.py --command="./npc --debug-tokens --file" --directory="test/samples/tokenization" --interactive

test_statements: debug
	./scripts/test.py --command="./npc --debug-statements --file" --directory="test/samples/statements"

test_statements_interactive: debug
	./scripts/test.py --command="./npc --debug-statements --file" --directory="test/samples/statements" --interactive
