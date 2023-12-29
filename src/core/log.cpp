module;

#include "core.h"
#include <cstdio>

export module core.log;

import core.string;
import core.tuple;
import core.memory;
import core.iterator;
import core.file;
import core.lock;

#define FORMAT_INT(type) \
template<> void format<type>(cref<type> arg, ref<fmtbuf> buf) { format<i64>((i64)arg, buf); }

export namespace core {
	using fmtbuf = buffer_base<BLOCK_1024>;
	template <typename T>
	void format(cref<T> arg, ref<fmtbuf> buf);

	template <u32 N>
	void format(const char (&arg)[N], ref<fmtbuf> buf) {
		for (int i : range(N)) {
			if (!arg[i]) return;
			buf.write((u8)arg[i]);
		}
	}

	struct format_string {
		using iterator = string::forward_iterator;

		template <typename... Args>
		format_string(cref<string> fmt, Args&&... args)
		: data(), buf(), beg(fmt.begin()), end(fmt.end()) {
			swallow(_format(args)...);
			_format();
		}

		~format_string() {
			// consider setting data to null to prevent destruction
		}

		ref<format_string> operator=(cref<format_string> other) {
			data = other.data;
		}

		template <typename T>
		int _format(cref<T> arg) {
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
		}

		void _format() {
			while (beg != end) {
				buf.write((u8)*beg);
				++beg;
			}

			buf.data[fmtbuf::size - 1] = 0;
			data = string((i8*)buf.data, buf.index);
			buf.data = nullptr;
		}

		string data;
		fmtbuf buf;
		iterator beg, end;
	};

	struct sink;
	struct logger {
		enum class severity {
			info, warn, crit
		};

		logger()
		: _sink() {
			JOLLY_CORE_ASSERT(_instance == nullptr);
			_sink = ptr_create<stdout_sink>().cast<sink>();
		}

		~logger() {
			_sink->flush();
		}

		void log(logger::severity level, cref<string> log) {
			switch (level) {
				case severity::info: {
					const char data[] = "info: ";
					_sink->write(memptr{ (u8*)data, sizeof(data) - 1 });
					break;
									 }
				case severity::warn: {
					const char data[] = "warn: ";
					_sink->write(memptr{ (u8*)data, sizeof(data) - 1 });
					break;
									 }
				case severity::crit: {
					const char data[] = "crit: ";
					_sink->write(memptr{ (u8*)data, sizeof(data) - 1 });
					break;
				 }
			}

			_sink->write(log);

			cstr newline = "\n";
			_sink->write(memptr{ (u8*)newline, 1 });
			_sink->flush();
		}


		inline void info(cref<format_string> fmt) {
			log(severity::info, fmt.data);
		}

		inline void warn(cref<format_string> fmt) {
			log(severity::warn, fmt.data);
		}

		inline void crit(cref<format_string> fmt) {
			log(severity::crit, fmt.data);
		}

		static ref<logger> logger::instance() {
			if (!_instance) {
				_instance = ptr_create<logger>();
			}

			return *_instance;
		}

		ptr<sink> _sink;

		static ptr<logger> _instance = nullptr;
	};

	struct sink {
		sink() = default;
		virtual ~sink() = default;

		virtual void write(memptr buf) = 0;
		virtual void flush() = 0;
	};

	struct stdout_sink : sink {
		stdout_sink() : _lock() {}
		~stdout_sink() {
			flush();
		}

		virtual void write(memptr buf) {
			lock l(_lock);
			fwrite(buf.data, sizeof(u8), buf.size, stdout);
		}

		virtual void flush() {
			fflush(stdout);
		}

		mutex _lock;
	};


	template<>
	void format<string>(cref<string> arg, ref<fmtbuf> buf) {
		buf.write(arg);
	}

	template <>
	void format<cstr>(cref<cstr> arg, ref<fmtbuf> buf) {
		i8* p = (i8*)arg;
		while (*p) {
			buf.write((u8)*p);
			p++;
		}
	}

	template<>
	void format<i8*>(cref<i8*> arg, ref<fmtbuf> buf) {
		format<cstr>((cstr)arg, buf);
	}

	template<>
	void format<i64>(cref<i64> arg, ref<fmtbuf> buf) {
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

		for (int i : range(data.index, 0, -1)) {
			i--;
			if (buf.index >= fmtbuf::size) return;
			buf.write((u8)data[i]);
		}
	}

	template<>
	void format<bool>(cref<bool> arg, ref<fmtbuf> buf) {
		if (arg) {
			format("true", buf);
		} else {
			format("false", buf);
		}
	}

	FORMAT_INT(i8);
	FORMAT_INT(i16);
	FORMAT_INT(i32);
	FORMAT_INT(u8);
	FORMAT_INT(u16);
	FORMAT_INT(u32);
	FORMAT_INT(u64);
}
