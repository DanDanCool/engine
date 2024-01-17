module;

#include "core.h"

export module core.operations;
import core.types;
import core.iterator;

export namespace core {
	u32 fnv1a(u8* key, u32 size) {
		u32 hash = 0x811c9dc5;

		for (i32 i : range(size))
			hash = (hash ^ key[i]) * 0x01000193;

		return hash;
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
	struct operations_base {
		using type = T;

		static type copy(cref<type> src) {
			return type(src);
		}

		static void copy(cref<type> src, ref<T> dst) {
			dst = forward_data(copy(src));
		}

		static u32 hash(cref<type> key) {
			return fnv1a(bytes(key), sizeof(type));
		}

		static void swap(ref<type> a, ref<type> b) {
			T tmp = copy(a);
			a = copy(b);
			b = copy(tmp);
		}
	};

	template <typename T>
	struct operations: public operations_base<T> {};

	template <typename T>
	u32 hash(cref<T> key) {
		return operations<T>::hash(key);
	}

	template <typename T>
	void copy(cref<T> src, ref<T> dst) {
		operations<T>::copy(src, dst);
	}

	template <typename T>
	T copy(cref<T> src) {
		return operations<T>::copy(src);
	}

	template <typename T>
	void swap(ref<T> a, ref<T> b) {
		operations<T>::swap(a, b);
	}
};
