.PHONY: clean debug release regenerate test test_update run_test_symbol_hashmap

INSTALL_DIR ?= /usr

CC = cc

CFLAGS_COMMON = -Wall -Wextra -Wpedantic -std=c11 -Wno-missing-braces
DEBUG_CFLAGS = $(CFLAGS_COMMON) -g
RELEASE_CFLAGS = $(CFLAGS_COMMON) -O3

CPPFLAGS_COMMON = -I./src
DEBUG_CPPFLAGS = $(CPPFLAGS_COMMON) -DDEBUG=1 -DINSTALL_DIR=\"$(shell pwd)/build\"
RELEASE_CPPFLAGS = $(CPPFLAGS_COMMON) -DINSTALL_DIR=\"$(INSTALL_DIR)\"

ALL_SOURCES = $(wildcard src/*.c)
SOURCES := $(filter-out $(wildcard src/debug*.c), $(ALL_SOURCES))
OBJECTS = $(patsubst src/%.c, build/%.o, $(SOURCES))
OBJECTS_DEBUG = $(patsubst src/%.c, build/%_db.o, $(ALL_SOURCES))

INCLUDE_HEADERS = src/not_python.h src/np_hash.h
DEBUG_INCLUDE = $(patsubst src/%.h, build/include/%.h, $(INCLUDE_HEADERS))

LIB_SOURCE = src/not_python.c src/np_hash.c
LIB_OBJECTS = $(patsubst src/%.c, build/%.o, $(LIB_SOURCE))
DEBUG_LIB_OBJECTS = $(patsubst src/%.c, build/%_db.o, $(LIB_SOURCE))

debug: $(OBJECTS_DEBUG) $(DEBUG_INCLUDE) build/lib/not_python_db.a
	@-rm npc
	$(CC) $(DEBUG_CPPFLAGS) $(DEBUG_CFLAGS) -o npc $^

release: $(OBJECTS) build/lib/not_python.a
	@-rm npc
	$(CC) $(RELEASE_CPPFLAGS) $(RELEASE_CFLAGS) -o npc $^

clean:
	-rm -rf build
	-rm -rf debug
	-rm -rf codegen
	-rm -rf backup
	-rm -rf npc_build
	-rm -rf testdir
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

build/lib/not_python_db.a: $(DEBUG_LIB_OBJECTS)
	@mkdir -p build/lib
	ar -rc $@ $^

build/lib/not_python.a: $(LIB_OBJECTS)
	@mkdir -p build/lib
	ar -rc $@ $^

build/include/%.h: src/%.h
	@mkdir -p build/include
	cp $^ $@

build/%.o: src/%.c
	@mkdir -p build
	$(CC) $(RELEASE_CPPFLAGS) $(RELEASE_CFLAGS) -c $^ -o $@

build/%_db.o: src/%.c
	@mkdir -p build
	$(CC) $(DEBUG_CPPFLAGS) $(DEBUG_CFLAGS) -c $^ -o $@

install: release
	install -d $(INSTALL_DIR)/lib/
	install -m 644 build/lib/not_python.a $(INSTALL_DIR)/lib/
	install -d $(INSTALL_DIR)/include/
	install -m 644 src/not_python.h $(INSTALL_DIR)/include/
	install -m 644 src/np_hash.h $(INSTALL_DIR)/include/
	install -d $(INSTALL_DIR)/bin/
	install -m 777 npc $(INSTALL_DIR)/bin/

uninstall:
	-rm $(INSTALL_DIR)/lib/not_python.a
	-rm $(INSTALL_DIR)/include/not_python.h
	-rm $(INSTALL_DIR)/include/np_hash.h
	-rm $(INSTALL_DIR)/bin/npc

test: debug run_test_symbol_hashmap
	./scripts/test.py 

test_update: debug
	./scripts/test.py --interactive

test_symbol_hashmap: build/np_hash_db.o build/arena_db.o build/hashmap_db.o build/diagnostics_db.o test/test_symbol_hashmap.c
	$(CC) $(DEBUG_CFLAGS) -o $@ $^

run_test_symbol_hashmap: test_symbol_hashmap
	./$^
	@echo "PASS"
