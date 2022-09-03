#include "hash.h"
#define XXH_STATIC_LINKING_ONLY
#define XXH_IMPLEMENTATION
#define XXH_INLINE_ALL
#include "../3rdparty/xxhash.h"
// TODO: controls over seed
#define HASH_SEED 0

uint64_t
hash_bytes(void* data, size_t length)
{
    return XXH64(data, length, HASH_SEED);
}
