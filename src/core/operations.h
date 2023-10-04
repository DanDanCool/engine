#pragma once

#include "core.h"

namespace core {
	u32 fnv1a(u8* key, u32 size);
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

	template <typename T>
		cref<T> max(cref<T> a, cref<T> b) {
			bool val = a > b;
			return val ? a : b;
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
