#include "operations.h"
#include "iterator.h"

namespace core {
	u32 fnv1a(u8* key, u32 size) {
		u32 hash = 0x811c9dc5;

		for (i32 i : range(size))
			hash = (hash ^ key[i]) * 0x01000193;

		return hash | (hash == 0);
	}

	bool cmpeq(cstr a, cstr b) {
		cstr p1 = a, p2 = b;
		while (true) {
			if (*p1 != *p2) return false;
			if (*p1 == '\0') break;

			p1++;
			p2++;
		}

		return true;
	}
}
