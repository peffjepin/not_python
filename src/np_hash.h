#ifndef HASH_H
#define HASH_H

#include <stddef.h>
#include <stdint.h>

uint64_t hash_bytes(void* data, size_t length);

#endif
