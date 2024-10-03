module;

#include "core.h"

export module core.operations;
import core.types;

export namespace core {
	u32 fnv1a(cptr<u8> key, u32 size) {
		u32 hash = 0x811c9dc5;

		for (u32 i = 0; i < size; i++)
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

	// we define structs with static methods because structs have partial tempalte specialization
	template <typename T>
	struct op_hash_base {
		using type = T;

		static u32 hash(cref<type> key) {
			return fnv1a((ptr<u8>)&key, sizeof(type));
		}
	};

	template <typename T>
	struct op_mem_base {
		using type = T;

		static type copy(cref<type> src) {
			return type(src);
		}

		static void copy(cref<type> src, ref<T> dst) {
			dst = forward_data(copy(src));
		}

		static void swap(ref<type> a, ref<type> b) {
			T tmp = copy(a);
			a = copy(b);
			b = copy(tmp);
		}
	};

	template <typename T>
	struct op_mem: public op_mem_base<T> {};

	template <typename T>
	struct op_hash: public op_hash_base<T> {};

	template <typename T>
	u32 hash(cref<T> key) {
		return op_hash<T>::hash(key);
	}

	template <typename T>
	void copy(cref<T> src, ref<T> dst) {
		op_mem<T>::copy(src, dst);
	}

	template <typename T>
	T copy(cref<T> src) {
		return op_mem<T>::copy(src);
	}

	template <typename T>
	void swap(ref<T> a, ref<T> b) {
		op_mem<T>::swap(a, b);
	}
};
