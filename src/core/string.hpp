module;

#include "core.h"

export module core.string;
import core.types;
import core.memory;
import core.simd;
import core.iterator;
import core.operations;

import core.impl.ryu_f32;
import core.impl.ryu_f64;

export namespace core {
	template <typename T>
	struct stringview_base {
		using type = T;
		using this_type = stringview_base<T>;

		stringview_base() = default;

		stringview_base(cptr<type> str)
		: data(nullptr), size(0) {
			*this = str;
		}

		stringview_base(cptr<type> str, u32 sz)
		: data(nullptr), size(sz) {
			*this = str;
		}

		ref<this_type> operator=(cptr<type> str) {
			u32 count = 0;
			while (str[count]) {
				count++
			}

			data = str;
			size = count;

			return *this;
		}

		operator cptr<type>() const {
			return data;
		}

		bool operator==(cref<this_type> other) const {
			if (size != other.size) return false;
			return cmp8((ptr<u8>)data, (ptr<u8>)other.data, size));
		}

		auto begin() const {
			return iterator::rforward_seq(data, 0);
		}

		auto end() const {
			return iterator::rforward_seq(data, size);
		}

		auto rbegin() const {
			return iterator::rreverse_seq(data, size - 1);
		}

		auto rend() const {
			return iterator::rreverse_seq(data, -1);
		}

		// not guaranteed to be null terminated
		cptr<type> data;
		u32 size;
	};

	template<typename T>
	struct string_base {
		using type = T;
		using this_type = string_base<T>;

		string_base() = default;
		string_base(cptr<type> str)
		: data(nullptr), size(0) {
			*this = str;
		}

		string_base(ptr<type> str, u32 size)
		: data(str), size(size) {}

		string_base(stringview_base<type> str)
		: data(nullptr), size(0) {
			*this = str;
		}

		string_base(fwd<this_type> other)
		: data(nullptr), size(0) {
			*this = forward_data(other);
		}

		string_base(cref<this_type> other)
		: data(nullptr), size(0) {
			*this = other;
		}

		~string_base() {
			if (!data) return;
			free256((ptr<void>)data);
		}

		ref<this_type> operator=(fwd<this_type> other) {
			data = other.data;
			size = other.size;

			other.data = nullptr;
			other.size = 0;
			return *this;
		}

		ref<this_type> operator=(cref<this_type> other) {
			*this = forward_data(other.copy());
			return *this;
		}

		ref<this_type> operator=(cptr<type> str) {
			u32 count = 0;
			while (str[count]) {
				count++;
			}

			u32 bytes = (u32)((count + 1) * sizeof(type));
			auto ptr = alloc256(bytes);
			copy8((ptr<u8>)str, ptr.data, bytes);

			data = (ptr<type>)ptr.data;
			size = count;
			return *this;
		}

		ref<this_type> operator=(stringview_base<type> str) {
			u32 bytes = (u32)((str.size + 1) * sizeof(type));
			auto ptr = alloc256(bytes);
			copy8((ptr<u8>)str.data, ptr.data, (u32)(bytes - sizeof(type)));

			data = (ptr<type>)ptr.data;
			size = str.size;
			return *this;
		}

		template <typename S>
		string_base<S> cast() const {
			u32 bytes = (u32)((size + 1) * sizeof(S));
			auto ptr = alloc256(bytes);
			ptr<S> buf = (ptr<S>)ptr.data;
			for (auto i : range(bytes)) {
				buf[i] = (S)data[i];
			}

			return string_base<S>(buf, bytes);
		}

		type operator[](u32 idx) const {
			return data[idx];
		}

		bool operator==(cref<this_type> other) const {
			if (size != other.size) return false;
			return cmp256((ptr<u8>)data, (ptr<u8>)other.data, align_size256(size));
		}

		operator ptr<type>() const {
			return data;
		}

		operator membuf() const {
			return membuf{ (ptr<u8>)data, (u32)(size * sizeof(type)) };
		}

		operator stringview_base<type>() const {
			return stringview_base<type>(data, size);
		}

		this_type copy() const {
			u32 bytes = (u32)((size + 1) * sizeof(type));
			auto ptr = alloc256(bytes);
			copy256((u8*)data, ptr.data, align_size256(bytes));
			return this_type((type*)ptr.data, size);
		}

		auto begin() const {
			return iterator::wforward_seq(data, 0);
		}

		auto end() const {
			return iterator::wforward_seq(data, size);
		}

		auto rbegin() const {
			return iterator::wreverse_seq(data, size - 1);
		}

		auto rend() const {
			return iterator::wreverse_seq(data, -1);
		}

		ptr<type> data;
		u32 size;
	};

	template <typename T>
	struct op_mem<string_base<T>> {
		using type = T;
		using string_type = string_base<T>;

		static string_type copy(cref<string_type> src) {
			return src.copy();
		}

		static void swap(ref<string_type> a, ref<string_type> b) {
			string_type tmp(a.data, a.size);
			a = forward_data(string_type(b.data, b.size));
			b = forward_data(tmp);
		}
	};

	template <typename T>
	struct op_hash<string_base<T>> {
		using type = T;
		using string_type = string_base<T>;

		static u32 hash(cref<string_type> key) {
			return fnv1a((cptr<u8>)key.data, (u32)(key.size * sizeof(type)));
		}
	};

	template <typename T>
	struct op_hash<stringview_base<T>> {
		using type = T;
		using string_type = stringview_base<T>;

		static u32 hash(cref<string_type> key) {
			return fnv1a((cptr<u8>)key.data, (u32)(key.size * sizeof(type)));
		}
	};

	typedef string_base<i8> string;
	typedef stringview_base<i8> stringview;
};
