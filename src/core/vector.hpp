module;

#include "core.h"
#include <initializer_list>

export module core.vector;
import core.types;
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

		vector(fwd<this_type> other)
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

		ref<this_type> operator=(fwd<this_type> other) {
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
			auto ptr = alloc256(sz * sizeof(type));
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
			auto ptr = alloc256(sz * sizeof(type));
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
			copy8((ptr<u8>)&data[size], (ptr<u8>)&data[idx], sizeof(type));
			zero8((ptr<u8>)&data[size], sizeof(type));
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

		auto begin() const {
			return iterator::wforward_seq(data, 0);
		}

		auto end() const {
			return iterator::wforward_seq(data, size);
		}

		auto rbegin() const {
			return iterator::wreverse_seq(*this, size - 1);
		}

		auto rend() const {
			return iterator::wreverse_seq(*this, -1);
		}

		ptr<type> data;
		u32 reserve;
		u32 size;
	};

	template<typename T, u32 N>
	struct array {
		static constexpr u32 size = N;
		using type = T;
		using this_type = array<T, N>;

		array() : data(), index(0) {
			zero8((ptr<u8>)data, sizeof(data));
		}

		void add(cref<type> val) {
			data[index++] = val;
		}

		cref<type> operator[](u32 idx) const {
			return data[idx];
		}

		auto begin() const {
			return iterator::wforward_seq(&data[0], 0);
		}

		auto end() const {
			return iterator::wforward_seq(&data[0], index);
		}

		auto rbegin() const {
			return iterator::wreverse_seq(&data[0], index - 1);
		}

		auto rend() const {
			return iterator::wreverse_seq(&data[0], -1);
		}

		type data[size];
		u32 index;
	};


	template <typename T>
	struct op_mem<vector<T>> {
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
