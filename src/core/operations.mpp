module;

#include "core.h"

export module core.operations;
import core.iterator;

export namespace core {
	u32 fnv1a(u8* key, u32 size) {
		u32 hash = 0x811c9dc5;

		for (i32 i : range(size))
			hash = (hash ^ key[i]) * 0x01000193;

		return hash | (hash == 0);
	}

	template <typename T>
		void copy(cref<T> src, ref<T> dst) {
			dst = src;
		}

	template <typename T>
		void swap(ref<T> a, ref<T> b) {
			T tmp{};
			copy(a, tmp);
			copy(b, a);
			copy(tmp, b);
		}

	template <typename T>
		T abs(cref<T> a) {
			return a < 0 ? -a : a;
		}

	template <typename T>
		cref<T> min(cref<T> a, cref<T> b) {
			bool val = a < b;
			return val ? a : b;
		}

	template <typename T1, typename T2>
		bool cmpeq(cref<T1> a, cref<T2> b) {
			return a == b;
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

	template <typename T>
		cref<T> max(cref<T> a, cref<T> b) {
			bool val = a > b;
			return val ? a : b;
		}

	template <typename T>
		T clamp(T a, T min, T max) {
			T tmp = a;
			tmp = tmp < min ? min : tmp;
			tmp = tmp > max ? max : tmp;
			return tmp;
		}

	template <typename T>
		struct hash_info {
			static u32 hash(cref<T> key) {
				return fnv1a(bytes(key), sizeof(T));
			}
		};

	template <typename T>
		u32 hash(cref<T> key) {
			return hash_info<T>::hash(key);
		}
};
