module;

#include "core.h"

export module core.string;
import core.memory;
import core.simd;
import core.operations;
import core.iterator;

export namespace core {
	template<typename T>
	struct string_base {
		using type = T;
		using this_type = string_base<T>;

		string_base() = default;
		string_base(const type* str)
		: data(nullptr), size(0) {
			*this = str;
		}

		string_base(type* str, u64 size)
		: data(str), size(size) {}

		string_base(this_type&& other)
		: data(nullptr), size(0) {
			*this = forward_data(other);
		}

		string_base(cref<this_type> other)
		: data(nullptr), size(0) {
			*this = other;
		}

		~string_base() {
			if (!data) return;
			free256((void*)data);
		}

		ref<this_type> operator=(this_type&& other) {
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

		ref<this_type> operator=(const type* str) {
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

		bool operator==(cref<this_type> other) const {
			if (size != other.size) return false;
			return cmp256((u8*)data, (u8*)other.data, align_size256((u32)size));
		}

		operator type*() const {
			return data;
		}

		operator memptr() const {
			return memptr{ (u8*)data, size * sizeof(type) };
		}

		this_type copy() const {
			u32 bytes = (u32)((size + 1) * sizeof(type));
			memptr ptr = alloc256(bytes);
			copy256((u8*)data, ptr.data, align_size256(bytes));
			return forward_data(this_type((type*)data, size));
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
				iterator_base::data++;
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
				iterator_base::data--;
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
	struct string_view_base {
		using type = T;

		type* data;
		u64 size;
	};

	template <typename T>
	struct operations<string_base<T>>: public operations_base<string_base<T>> {
		using type = T;
		using string_type = string_base<T>;

		static string_type copy(cref<string_type> src) {
			return src.copy();
		}

		static u32 hash(cref<string_type> key) {
			return fnv1a((u8*)key.data, (u32)(key.size * sizeof(type)));
		}

		static void swap(ref<string_type> a, ref<string_type> b) {
			string_type tmp(a.data, a.size);
			a = forward_data(string_type(b.data, b.size));
			b = forward_data(tmp);
		}
	};

	typedef string_base<i8> string;
	typedef string_view_base<i8> string_view;
};
