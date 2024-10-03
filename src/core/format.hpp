module;

#include <core/core.h>

export module core.format;
import core.types;
import core.simd;
import core.memory;
import core.string;
import core.tuple;
import core.iterator;
import core.impl.ryu_f32;
import core.impl.ryu_f64;

#define FORMAT_INT(type) \
void format(type arg, ref<buffer> buf) { format((i64)arg, buf); }

export namespace core {
	void format(cref<string> arg, ref<buffer> buf) {
		buf.write(arg);
	}

	void format(cstr arg, ref<buffer> buf) {
		u32 i = 0;
		while (arg[i]) {
			buf.write((u8)arg[i]);
			p++;
		}
	}

	void format(strv arg, ref<buffer> buf) {
		for (auto i : range(arg.))
	}

	void format(i8 arg, ref<buffer> buf) {
		buf.write((u8)arg);
	}

	void format(i64 arg, ref<buffer> buf) {
		if (arg == 0) {
			buf.write('0');
			return;
		}

		array<i8, 20> data;

		i64 tmp = abs(arg);
		i64 div = 10;

		while (tmp) {
			i64 val = tmp % div;
			tmp -= val;
			data.add('0' + (i8)val);
			tmp /= div;
		}

		if (arg < 0) {
			buf.write((u8)'-');
		}

		for (i8 digit : reverse(data)) {
			if (buf.index >= buffer::size) return;
			buf.write((u8)digit);
		}
	}

	void format(bool arg, ref<buffer> buf) {
		if (arg) {
			format("true", buf);
		} else {
			format("false", buf);
		}
	}

	void format(f64 arg, ref<buffer> buf) {
		core::impl::ryu::ryu_f64_dtos(arg, buf);
	}

	void format(f32 arg, ref<buffer> buf) {
		core::impl::ryu::ryu_f32_ftos(arg, buf);
	}

	FORMAT_INT(i16);
	FORMAT_INT(i32);
	FORMAT_INT(u8);
	FORMAT_INT(u16);
	FORMAT_INT(u32);
	FORMAT_INT(u64);

	template <typename... Args>
	string format_string(cref<string> fmt, fwd<Args>... args) {
		auto beg = fmt.begin();
		auto end = fmt.end();
		buffer buf;

		auto helper = [&](auto&& arg) {
			while (beg != end) {
				if (*beg == '%') {
					++beg;
					break;
				}

				buf.write((u8)*beg);
				++beg;
			}

			format(arg, buf);
			return 0;
		};

		(helper(args), ...);
		while (beg != end) {
			buf.write((u8)*beg);
			++beg;
		}

		buf.write(0);
		ptr<i8> data = (ptr<i8>)buf.data.data;
		buf.data = nullptr;
		return string(data, buf.index);
	}

	i64 stoi(cstr in) {
		auto transform10 = [](i8 in) {
			i8 tmp = in - '0';
			JOLLY_CORE_ASSERT(tmp < 10);
			return tmp;
		};

		auto transform16 = [](i8 in) {
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

		bool sign = in[0] == '-';
		i64 beg = 0 + (in[0] == '-') + (in[0] == '+');
		bool hex = in[beg] == '0' && in[beg + 1] == 'x';

		auto transform = hex ? transform16 : transform10;
		beg += hex ? 2 : 0;

		// maximum length of a number that can be encoded by i64 is 19 characters
		array<i8, 20> data;
		for (i32 i : range((i32)beg, BLOCK_64)) {
			if (!in[i]) break;
			if (in[i] == '_') continue;

			data.add(transform(in[i]));
			if (data.index >= 20) break;
		}

		i64 num = 0;
		i64 mul = 1;
		i64 base = hex ? 16 : 10;
		for (i64 digit : reverse(data)) {
			num += digit * mul;
			mul *= base;
		}

		return sign ? -num : num;
	}

	f64 stod(cstr in) {
		return core::impl::ryu::ryu_f64_stod(in);
	}

	f32 stof(cstr in) {
		return core::impl::ryu::ryu_f32_stof(in);
	}
}
