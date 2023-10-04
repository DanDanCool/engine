#pragma once

#include "core.h"
#include "simd.h"
#include "memory.h"
#include "operations.h"

namespace core {
	template<typename T>
	struct string_base {
		using type = T;

		string_base() = default;
		string_base(const type* str)
		: data(nullptr), size(0) {
			*this = str;
		}

		string_base(type* str, u64 size)
		: data(str), size(size) {}

		string_base(cref<string_base> other)
		: data(nullptr), size(0) {
			*this = other;
		}

		string_base(string_base&& other)
		: data(nullptr), size(0) {
			*this = other;
		}

		~string_base() {
			if (!data) return;
			free256((void*)data);
		}

		ref<string_base> operator=(cref<string_base> other) {
			u32 bytes = (u32)((other.size + 1) * sizeof(type));
			memptr ptr = alloc256(bytes);
			copy256((u8*)other.data, ptr.data, align_size256(bytes));
			data = (type*)ptr.data;
			size = other.size;
			return *this;
		}

		ref<string_base> operator=(string_base&& other) {
			data = other.data;
			size = other.size;

			other.data = nullptr;
			other.size = 0;
			return *this;
		}

		ref<string_base> operator=(const type* str) {
			u64 count = 0;
			while (str[count]) {
				count++;
			}

			u32 bytes = (u32)((count + 1) * sizeof(type));
			memptr ptr = alloc256(bytes);
			copy8((u8*)str, ptr.data, bytes);

			data = (type*)ptr.data;
			size = count;
			return *this;
		}

		template <typename S>
		string_base<S> cast() const {
			u32 bytes = (u32)((size + 1) * sizeof(S));
			memptr ptr = alloc256(bytes);
			S* buf = (S*)ptr.data;
			for (int i : range(bytes)) {
				buf[i] = (S)data[i];
			}

			return string_base<S>(buf, bytes);
		}

		cref<type> operator[](u32 idx) const {
			return cref(data[idx]);
		}

		bool operator==(cref<string_base> other) const {
			if (size != other.size) return false;

			u32 count = align_size256((u32)size) / BLOCK_32;
			u8* a = (u8*)data;
			u8* b = (u8*)other.data;
			for (u32 i = 0; i < count; i++) {
				__m256i x = _mm256_load_si256((__m256i*)a);
				__m256i y = _mm256_load_si256((__m256i*)b);

				__m256i z = _mm256_cmpeq_epi8(x, y);
				int equal = _mm256_testc_si256(z, z);
				if (!equal) return false;

				a += BLOCK_32;
				b += BLOCK_32;
			}

			return true;
		}

		operator type*() const {
			return data;
		}

		operator memptr() const {
			return memptr{ (u8*)data, size * sizeof(type) };
		}

		struct iterator_base {
			iterator_base(type* in) : data(in) {}

			bool operator!=(cref<iterator_base> other) const {
				return data != other.data;
			}

			ref<type> operator*() const {
				return *data;
			}

			type* data;
		};

		struct forward_iterator : public iterator_base {
			forward_iterator(type* in) : iterator_base(in) {}
			forward_iterator& operator++() {
				data++;
				return *this;
			}
		};

		forward_iterator begin() const {
			return forward_iterator(&data[0]);
		}

		forward_iterator end() const {
			return forward_iterator(&data[size]);
		}

		struct reverse_iterator : public iterator_base {
			reverse_iterator(type* in): iterator_base(in) {}
			reverse_iterator& operator++() {
				data--;
				return *this;
			}
		};

		reverse_iterator rbegin() const {
			return reverse_iterator(&data[size - 1]);
		}

		reverse_iterator rend() const {
			return reverse_iterator(&data[-1]);
		}

		type* data;
		u64 size;
	};

	template <typename T>
		struct hash_info<string_base<T>> {
			static u32 hash(cref<string_base<T>> key) {
				return fnv1a((u8*)key.data, (u32)(key.size * sizeof(T)));
			}
		};

	typedef string_base<i8> string;
};
