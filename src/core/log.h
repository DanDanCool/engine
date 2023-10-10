#pragma once

#include "core.h"
#include "string.h"
#include "tuple.h"
#include "memory.h"
#include "iterator.h"

#define LOG_INFO(...) core::logger::instance().info(core::format_string(__VA_ARGS__))
#define LOG_WARN(...) core::logger::instance().warn(core::format_string(__VA_ARGS__))
#define LOG_CRIT(...) core::logger::instance().crit(core::format_string(__VA_ARGS__))

namespace core {
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
		format_string(cref<string> fmt, Args&&... args) : data(), buf() {
			_format(fmt.begin(), fmt.end(), args...);
		}

		~format_string() {
			// string data holds the memory, see _format
			buf.data = nullptr;
		}

		ref<format_string> operator=(cref<format_string> other) {
			data = other.data;
		}

		template <typename T, typename... Args>
		void _format(ref<iterator> beg, cref<iterator> end, cref<T> arg, Args&&... args) {
			while (beg != end) {
				if (*beg == '%') {
					++beg;
					break;
				}

				buf.write((u8)*beg);
				++beg;
			}

			format(arg, buf);
			_format(beg, end, args...);
		}

		void _format(ref<iterator> beg, cref<iterator> end) {
			while (beg != end) {
				buf.write((u8)*beg);
				++beg;
			}

			buf.data.data[fmtbuf::size - 1] = 0;
			data = string((i8*)buf.data.data, buf.index);
		}

		string data;
		fmtbuf buf;
	};

	struct sink;
	struct logger {
		logger();
		~logger();

		enum class severity {
			info, warn, crit
		};

		void log(severity level, cref<string> log);

		inline void info(cref<format_string> fmt) {
			log(severity::info, fmt.data);
		}

		inline void warn(cref<format_string> fmt) {
			log(severity::warn, fmt.data);
		}

		inline void crit(cref<format_string> fmt) {
			log(severity::crit, fmt.data);
		}

		static ref<logger> instance();

		ptr<sink> _sink;

		static ptr<logger> _instance;
	};
};
