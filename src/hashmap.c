#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "diagnostics.h"
#include "np_hash.h"
#include "syntax.h"

#define LOOKUP_CAPACITY_ELEMENTS_CAPACITY_RATIO 2

static void rehash(SymbolHashmap* hm);

/*
we want: elements_capacity * RATIO = lookup_capacity

1 element_capacity requires: sizeof(Symbol) bytes
1 lookup_capacity requires: sizeof(int) bytes
1 general unit of capacity requires: sizeof(Symbol) + RATIO*sizeof(int)

we know how many bytes are available so:

general_capacity = bytes / requirement_for_1_general_unit_of_capacity
elements_allocation = general_capacity * sizeof(Symbol)
lookup_allocation = general_capacity * RATIO * sizeof(int)
*/
static void
layout_hm_memory(SymbolHashmap* hm)
{
    size_t general_capacity =
        hm->bytes /
        ((sizeof(Symbol) + (LOOKUP_CAPACITY_ELEMENTS_CAPACITY_RATIO * sizeof(int))));
    bool rehash_required = (hm->elements != NULL);

    hm->elements = (Symbol*)hm->buffer;
    hm->elements_capacity = general_capacity;
    hm->lookup_table = (int*)(hm->buffer + (general_capacity * sizeof(Symbol)));
    hm->lookup_capacity = LOOKUP_CAPACITY_ELEMENTS_CAPACITY_RATIO * general_capacity;

    memset(hm->lookup_table, -1, hm->lookup_capacity * sizeof(int));

    if (rehash_required) rehash(hm);
}

SymbolHashmap
symbol_hm_init(Arena* arena)
{
    SymbolHashmap hm = {
        .arena = arena, .bytes = 1024, .buffer = arena_dynamic_alloc(arena, 1024)};
    layout_hm_memory(&hm);
    return hm;
}

// used when probing the lookup table
static SourceString
key_at(SymbolHashmap* hm, size_t probe)
{
    return hm->elements[hm->lookup_table[probe]].identifier;
}

static bool
hm_lookup_insert(SymbolHashmap* hm, size_t elem_index, SourceString key)
{
    uint64_t hash = hash_bytes((void*)key.data, key.length);
    size_t probe = hash % hm->lookup_capacity;
    size_t init = probe;
    for (;;) {
        if (hm->lookup_table[probe] < 0) {
            hm->lookup_table[probe] = elem_index;
            return true;
        }
        else if (SOURCESTRING_EQ(key_at(hm, probe), key)) {
            // for now we're only tracking the first occurence of a symbol within a scope
            return false;
        }
        if (probe == hm->lookup_capacity - 1)
            probe = 0;
        else
            probe++;
        assert(probe != init && "ERROR: linear probe looped on hm_put");
    }
}

static void
rehash(SymbolHashmap* hm)
{
    for (size_t i = 0; i < hm->elements_count; i++) {
        SourceString key = hm->elements[i].identifier;
        hm_lookup_insert(hm, i, key);
    }
}

void
symbol_hm_put(SymbolHashmap* hm, Symbol element)
{
    if (hm->finalized) {
        fprintf(stderr, "ERROR: symbol hashmap cannot be added to after finalization");
        exit(1);
    }
    if (hm->elements_count == hm->elements_capacity) {
        hm->bytes *= 2;
        hm->buffer = arena_dynamic_realloc(hm->arena, hm->buffer, hm->bytes);
        layout_hm_memory(hm);
    }
    size_t element_index = hm->elements_count;
    if (hm_lookup_insert(hm, element_index, element.identifier)) {
        // for now we're only tracking the first occurence of a symbol within a scope
        hm->elements[hm->elements_count++] = element;
    }
}

// NOTE: it is not safe to continue pushing elements into the hashmap while
// using the Symbol* returned from this function, as it might be moved by a resize
Symbol*
symbol_hm_get(SymbolHashmap* hm, SourceString identifier)
{
    uint64_t hash = hash_bytes((void*)identifier.data, identifier.length);
    size_t probe = hash % hm->lookup_capacity;
    size_t init = probe;

    for (;;) {
        int element_index = hm->lookup_table[probe];
        if (element_index < 0) {
            return NULL;
        }
        Symbol* element = hm->elements + element_index;
        if (SOURCESTRING_EQ(element->identifier, identifier)) {
            return element;
        }
        if (probe == hm->lookup_capacity - 1)
            probe = 0;
        else
            probe++;
        assert(probe != init && "ERROR: linear probe looped on hm_get");
    }
}

void
symbol_hm_finalize(SymbolHashmap* hm)
{
    size_t total_bytes_required =
        (sizeof(int) * hm->lookup_capacity) + (sizeof(Symbol) * hm->elements_count);
    uint8_t* static_buffer = arena_alloc(hm->arena, total_bytes_required);
    size_t elements_size = sizeof(Symbol) * hm->elements_count;
    size_t lookup_size = sizeof(int) * hm->lookup_capacity;
    memcpy(static_buffer, hm->elements, elements_size);
    memcpy(static_buffer + elements_size, hm->lookup_table, lookup_size);

    hm->elements = (Symbol*)static_buffer;
    hm->lookup_table = (int*)(static_buffer + elements_size);

    arena_dynamic_free(hm->arena, hm->buffer);
    hm->finalized = true;
}
