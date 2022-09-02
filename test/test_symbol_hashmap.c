#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/syntax.h"

static void
assert_elems_eq(Symbol* elem1, Symbol* elem2)
{
    assert(strcmp(elem1->variable->identifier, elem2->variable->identifier) == 0);
    assert(elem1->kind == elem2->kind);
}

static Symbol
make_test_symbol(char* ident)
{
    Variable* sp = malloc(sizeof(Variable));
    sp->identifier = ident;
    return (Symbol){.kind = SYM_VARIABLE, .variable = sp};
}

static void
test_basic_put_get(void)
{
    Arena* arena = arena_init();

    SymbolHashmap hm = symbol_hm_init(arena);
    Symbol elem1 = make_test_symbol("ident1");
    Symbol elem2 = make_test_symbol("ident2");
    Symbol elem3 = make_test_symbol("ident3");

    symbol_hm_put(&hm, elem1);
    symbol_hm_put(&hm, elem2);
    symbol_hm_put(&hm, elem3);

    assert_elems_eq(&elem1, symbol_hm_get(&hm, elem1.variable->identifier));
    assert_elems_eq(&elem2, symbol_hm_get(&hm, elem2.variable->identifier));
    assert_elems_eq(&elem3, symbol_hm_get(&hm, elem3.variable->identifier));

    arena_free(arena);
}

static void
test_get_after_finalization(void)
{
    Arena* arena = arena_init();

    SymbolHashmap hm = symbol_hm_init(arena);
    Symbol elem1 = make_test_symbol("ident1");
    Symbol elem2 = make_test_symbol("ident2");
    Symbol elem3 = make_test_symbol("ident3");

    symbol_hm_put(&hm, elem1);
    symbol_hm_put(&hm, elem2);
    symbol_hm_put(&hm, elem3);
    symbol_hm_finalize(&hm);

    assert_elems_eq(&elem1, symbol_hm_get(&hm, elem1.variable->identifier));
    assert_elems_eq(&elem2, symbol_hm_get(&hm, elem2.variable->identifier));
    assert_elems_eq(&elem3, symbol_hm_get(&hm, elem3.variable->identifier));

    arena_free(arena);
}

static void
test_put_get_after_growing(void)
{
    Arena* arena = arena_init();

    SymbolHashmap hm = symbol_hm_init(arena);
    int count = hm.elements_capacity + 1;
    Symbol elems[count];
    for (int i = 0; i < count; i++) {
        char* ident = calloc(1, 20);
        sprintf(ident, "ident%i", i);
        elems[i] = make_test_symbol(ident);
        symbol_hm_put(&hm, elems[i]);
    }
    assert(hm.elements_capacity > (size_t)count);

    for (int i = 0; i < count; i++) {
        assert_elems_eq(elems + i, symbol_hm_get(&hm, elems[i].variable->identifier));
    }

    arena_free(arena);
}

static void
test_get_after_growing_and_finalizing(void)
{
    Arena* arena = arena_init();

    SymbolHashmap hm = symbol_hm_init(arena);
    int count = hm.elements_capacity + 1;
    Symbol elems[count];
    for (int i = 0; i < count; i++) {
        char* ident = calloc(1, 20);
        sprintf(ident, "ident%i", i);
        elems[i] = make_test_symbol(ident);
        symbol_hm_put(&hm, elems[i]);
    }
    assert(hm.elements_capacity > (size_t)count);
    symbol_hm_finalize(&hm);

    for (int i = 0; i < count; i++) {
        assert_elems_eq(elems + i, symbol_hm_get(&hm, elems[i].variable->identifier));
    }

    arena_free(arena);
}

int
main(void)
{
    test_basic_put_get();
    test_get_after_finalization();
    test_put_get_after_growing();
    test_get_after_growing_and_finalizing();
}
