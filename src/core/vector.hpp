module;

#include "core.h"
#include <initializer_list>

export module core.vector;
import core.traits;
import core.memory;
import core.simd;
import core.iterator;
import core.operations;

export namespace core {
	constexpr u32 VECTOR_DEFAULT_SIZE = BLOCK_32;
	template<typename T>
	struct vector {
		using type = T;
		using this_type = vector<type>;

		vector() = default;
		vector(u32 sz)
		: data(nullptr), reserve(0), size(0) {
			_allocate(sz);
		}

		vector(this_type&& other)
		: data(nullptr), reserve(0), size(0) {
			*this = forward_data(other);
		}

		vector(cref<this_type> other)
		: data(nullptr), reserve(0), size(0) {
			*this = other;
		}

		vector(std::initializer_list<type> l)
		: data(nullptr), reserve(0), size(0) {
			*this = l;
		}

		~vector() {
			if (!data) return;
			destroy();
		}

		ref<this_type> operator=(this_type&& other) {
			data = other.data;
			reserve = other.reserve;
			size = other.size;

			other.data = nullptr;
			other.reserve = 0;
			other.size = 0;

			return *this;
		}

		ref<this_type> operator=(cref<this_type> other) {
			*this = forward_data(other.copy());
			return *this;
		}

		ref<this_type> operator=(std::initializer_list<type> l) {
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
				core::destroy(&data[i]);
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

		void add(type&& val) {
			if (reserve <= size) {
				resize(reserve * 2);
			}

			data[size++] = forward_data(val);
		}

		// this implementation copies the argument
		void add(cref<type> val) {
			add(forward_data(core::copy(val)));
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
			core::destroy(&data[idx]);
			copy8(bytes(data[size]), bytes(data[idx]), sizeof(type));
			zero8(bytes(data[size]), sizeof(type));
		}

		u32 find(cref<type> other) const {
			for (u32 i : range(size)) {
				if (cmpeq(data[i], other)) {
					return i;
				}
			}

			return U32_MAX;
		}

		this_type copy() const {
			this_type tmp(reserve);
			for (auto& item : *this) {
				tmp.add(item);
			}

			return tmp;
		}

		ref<type> operator[](u32 idx) const {
			return data[idx];
		}

		struct iterator_base {
			iterator_base(cref<this_type> in, u32 idx)
			: data(in), index(idx) {}

			bool operator!=(cref<iterator_base> other) const {
				return index != other.index;
			}

			ref<type> operator*() const {
				return data[index];
			}

			cref<this_type> data;
			u32 index;
		};

		struct forward_iterator : public iterator_base {
			using iterator_base::iterator_base;
			ref<forward_iterator> operator++() {
				iterator_base::index++;
				return *this;
			}
		};

		struct reverse_iterator : public iterator_base {
			using iterator_base::iterator_base;
			ref<reverse_iterator> operator++() {
				iterator_base::index--;
				return *this;
			}
		};

		forward_iterator begin() const {
			return forward_iterator(*this, 0);
		}

		forward_iterator end() const {
			return forward_iterator(*this, size);
		}

		reverse_iterator rbegin() const {
			return reverse_iterator(*this, size - 1);
		}

		reverse_iterator rend() const {
			return reverse_iterator(*this, -1);
		}

		type* data;
		u32 reserve;
		u32 size;
	};

	template <typename T>
	struct operations<vector<T>>: public operations_base<vector<T>> {
		using type = T;
		using vector_type = vector<T>;

		static vector_type copy(cref<vector_type> src) {
			return src.copy();
		}

		static void swap(ref<vector_type> a, ref<vector_type> b) {
			vector_type tmp = forward_data(a);
			a = forward_data(b);
			b = forward_data(tmp);
		}
	};
}
