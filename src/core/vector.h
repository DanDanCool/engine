#pragma once

#include "core.h"
#include "traits.h"
#include "memory.h"
#include "iterator.h"
#include "operations.h"

#include <initializer_list>

namespace core {
	const u32 VECTOR_DEFAULT_SIZE = 32;
	template<typename T>
	struct vector {
		using type = T;

		vector() = default;
		vector(u32 sz)
		: data(nullptr), reserve(0), size(0) {
			_allocate(sz);
		}

		vector(vector<type>&& other)
		: data(nullptr), reserve(0), size(0) {
			*this = other;
		}

		vector(cref<vector<type>>& other)
		: data(other.data), reserve(other.reserve), size(other.size) {}

		vector(std::initializer_list<type> l)
		: data(nullptr), reserve(0), size(0) {
			*this = l;
		}

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

		ref<vector<type>> operator=(std::initializer_list<type> l) {
			_allocate(0);
			for (auto& item : l) {
				add(item);
			}

			return *this;
		}

		void _allocate(u32 sz) {
			sz = max<u32>(sz, VECTOR_DEFAULT_SIZE);
			memptr ptr = alloc256(sz * sizeof(type));
			data = (type*)ptr.data;
			reserve = (u32)ptr.size / sizeof(type);
		}

		void destroy() {
			for (int i : range(size)) {
				_cleanup<is_destructible_v<type>>(data[i]);
			}

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
			reserve = (u32)(ptr.size / sizeof(type));
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

		void del(u32 idx) {
			size--;
			_cleanup<is_destructible_v<type>>(data[idx]);
			copy8(bytes(data[size]), bytes(data[idx]), sizeof(type));
			zero8(bytes(data[size]), sizeof(type));
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
			ref<forward_iterator> operator++() {
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
			ref<reverse_iterator> operator++() {
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

		template<bool val> void _cleanup(ref<type> obj) {};
		template<> void _cleanup<true>(ref<type> obj) {
				obj.~type();
		}

		type* data;
		u32 reserve;
		u32 size;
	};
}
