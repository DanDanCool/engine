module;

#include "core.h"

export module core.string;
import core.types;
import core.memory;
import core.simd;
import core.iterator;
import core.operations;
import core.tuple;

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
			auto ptr = alloc256(bytes);
			copy8((u8*)str, ptr.data, bytes);

			data = (type*)ptr.data;
			size = count;
			return *this;
		}

		template <typename S>
		string_base<S> cast() const {
			u32 bytes = (u32)((size + 1) * sizeof(S));
			auto ptr = alloc256(bytes);
			S* buf = (S*)ptr.data;
			for (int i : range(bytes)) {
				buf[i] = (S)data[i];
			}

			return string_base<S>(buf, bytes);
		}

		type operator[](u32 idx) const {
			return data[idx];
		}

		bool operator==(cref<this_type> other) const {
			if (size != other.size) return false;
			return cmp256((u8*)data, (u8*)other.data, align_size256((u32)size));
		}

		operator type*() const {
			return data;
		}

		operator membuf() const {
			return membuf{ (u8*)data, size * sizeof(type) };
		}

		this_type copy() const {
			u32 bytes = (u32)((size + 1) * sizeof(type));
			auto ptr = alloc256(bytes);
			copy256((u8*)data, ptr.data, align_size256(bytes));
			return this_type((type*)ptr.data, size);
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

	i64 stoi_impl(cstr in, i64 base, auto transform, i32 end = BLOCK_64) {
		// maximum length of a number that can be encoded by i64 is 19 characters
		array<i8, 20> data;
		for (i32 i : range( end)) {
			if (!in[i]) break;
			if (in[i] == '_') continue;

			data.add(transform(in[i]));
			if (data.index >= 20) break;
		}

		i64 num = 0;
		i64 mul = 1;
		for (i64 digit : reverse(data)) {
			num += digit * mul;
			mul *= base;
		}

		return num;
	}

	i64 stoi(cstr in) {
		i64 sign = in[0] == '-' ? -1 : 1;
		i64 beg = 0 + (in[0] == '-') + (in[0] == '+');

		auto transform = [](i8 in) {
			i8 tmp = in - '0';
			JOLLY_CORE_ASSERT(tmp < 10);
			return tmp;
		};

		return stoi_impl(in + beg, 10, transform) * sign;
	}

	i64 stoh(cstr in) {
		auto transform = [](i8 in) {
			i8 digit = in - '0';
			i8 upper = in - 'A';
			i8 lower = in - 'a';

			digit = digit < 0 ? I8_MAX : digit;
			upper = digit < 0 ? I8_MAX : upper + 10;
			lower = digit < 0 ? I8_MAX : lower + 10;

			i8 tmp = min(digit, min(upper, lower));
			JOLLY_CORE_ASSERT(tmp <= 0xf);
			return tmp;
		};

		i64 sign = in[0] == '-' ? -1 : 1;
		i64 beg = 0 + (in[0] == '-') + (in[0] == '+');

		JOLLY_CORE_ASSERT(in[beg] == '0');
		JOLLY_CORE_ASSERT(in[beg + 1] == 'x');

		return stoi_impl(in + beg + 2, 16, transform) * sign;
	}

	f64 stod(cstr in) {
		auto transform = [](i8 in) {
			i8 tmp = in - '0';
			JOLLY_CORE_ASSERT(tmp < 10);
			return tmp;
		};

		i32 deci = -1;
		i32 expo = -1;
		i32 size = -1;

		for (i32 i : range(BLOCK_64)) {
			if (!in[i]) {
				size = i;
				break;
			}
			if (in[i] == '.')
				deci = i;
			if (in[i] == 'e')
				expo = i;
		}

		i64 sign = in[0] == '-' ? -1 : 1;
		f64 whole = 0, fract = 0, power = 1;

		i64 beg = (in[0] == '-') + (in[0] == '+');
		i64 end = (deci == -1 ? size : deci) - beg;
		whole = (f64)stoi_impl(in + beg, 10, transform, end);

		if (deci != -1) {
			i64 offset = deci + 1;
			i64 end = (expo == -1 ? size : expo) - offset;
			fract = (f64)stoi_impl(in + offset, 10, transform, end);

			f64 div = 1;
			for (i32 i : range(end)) {
				div *= 10;
			}

			fract /= div;
		}

		if (expo != -1) {
			i64 sign = in[expo + 1] == '-' ? -1 : 1;
			i64 beg = 1 + (in[expo + 1] == '-') + (in[expo + 1] == '+');
			i64 count = stoi_impl(in + beg + expo, 10, transform);

			for (i32 i : range((i32)count)) {
				power *= 10;
			}

			if (sign == -1) {
				power = 1 / power;
			}
		}

		return (whole + fract) * power * sign;
	}
};
