#include "operations.h"
#include "iterator.h"

u32 core::fnv1a(u8* key, u32 size) {
	u32 hash = 0x811c9dc5;

	for (i32 i : range(size))
		hash = (hash ^ key[i]) * 0x01000193;

	return hash | (hash == 0);
}
