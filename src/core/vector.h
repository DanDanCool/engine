#pragma once

#include "core.h"
#include "traits.h"
#include "memory.h"
#include "iterator.h"
#include "operations.h"

namespace core {
	const u32 VECTOR_DEFAULT_SIZE = 32;
	template<typename T>
	struct vector {
		using type = T;

		vector() = default;
		vector(u32 sz)
		: data(nullptr), reserve(0), size(0) {
			sz = max<u32>(sz, VECTOR_DEFAULT_SIZE);
			memptr ptr = alloc256(sz * sizeof(type));
			data = (type*)ptr.data;
			reserve = (u32)ptr.size / sizeof(type);
		}

		vector(vector<type>&& other)
		: data(other.data), reserve(other.reserve), size(other.size) {
			other.data = nullptr;
			other.reserve = 0;
			other.size = 0;
		}

		vector(cref<vector<type>>& other)
		: data(other.data), reserve(other.reserve), size(other.size) {}

		~vector() {
			if (!data) return;
			destroy();
		}

		ref<vector<type>> operator=(vector<type>&& other) {
			data = other.data;
			reserve = other.reserve;
			size = other.size;

			other.data = nullptr;
			other.reserve = 0;
			other.size = 0;

			return *this;
		}

		ref<vector<type>> operator=(cref<vector<type>> other) {
			data = other.data;
			reserve = other.reserve;
			size = other.size;

			return *this;
		}

		void destroy() {
			cleanup<is_destructible<type>::value>();
			free256((void*)data);
			data = nullptr;
			reserve = 0;
			size = 0;
		}

		void resize(u32 sz) {
			memptr ptr = alloc256(sz * sizeof(type));
			copy256((u8*)data, ptr.data, align_size256(reserve * sizeof(type)));
			free256((void*)data);
			data = (type*)ptr.data;
			reserve = ptr.size / sizeof(type);
		}

		void add(cref<type> val) {
			if (reserve <= size) {
				resize(reserve * 2);
			}

			copy<type>(val, data[size++]);
		}

		ref<type> add() {
			if (reserve <= size) {
				resize(reserve * 2);
			}

			u32 idx = size++;
			return data[idx];
		}

		ref<type> del(u32 idx) {
			size--;
			swap<type>(ref(data[idx]), ref(data[size]));
			return ref(data[size]);
		}

		ref<type> operator[](u32 idx) const {
			return data[idx];
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
		u32 reserve;
		u32 size;

		private:
		template<bool val> void cleanup() {};
		template<> void cleanup<true>() {
			for (i32 i : range(size)) {
				data[i].~type();
			}
		}
	};
}
