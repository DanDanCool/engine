module;

#include "core.h"
#include <cstdio>

export module core.log;
import core.types;
import core.string;
import core.tuple;
import core.memory;
import core.simd;
import core.iterator;
import core.file;
import core.lock;
import core.format;

export namespace core {
	struct sink {
		sink() = default;
		virtual ~sink() = default;

		virtual void write(membuf buf) = 0;
		virtual void flush() = 0;
	};

	struct stdout_sink : sink {
		stdout_sink() : _lock() {}
		~stdout_sink() {
			flush();
		}

		virtual void write(membuf buf) {
			lock l(_lock);
			fwrite(buf.data, sizeof(u8), buf.size, stdout);
		}

		virtual void flush() {
			fflush(stdout);
		}

		mutex _lock;
	};

	struct logger {
		enum class severity {
			info, warn, crit
		};

		logger()
		: _sink() {
			JOLLY_CORE_ASSERT(_instance == nullptr);
			_sink = mem_create<stdout_sink>().cast<sink>();
		}

		~logger() {
			_sink->flush();
		}

		void log(logger::severity level, cref<string> log) {
			switch (level) {
				case severity::info: {
					const char data[] = "info: ";
					_sink->write(membuf{ (u8*)data, sizeof(data) - 1 });
					break;
									 }
				case severity::warn: {
					const char data[] = "warn: ";
					_sink->write(membuf{ (u8*)data, sizeof(data) - 1 });
					break;
									 }
				case severity::crit: {
					const char data[] = "crit: ";
					_sink->write(membuf{ (u8*)data, sizeof(data) - 1 });
					break;
				 }
			}

			_sink->write(log);

			cstr newline = "\n";
			_sink->write(membuf{ (u8*)newline, 1 });
			_sink->flush();
		}


		inline void info(cref<string> fmt) {
			log(severity::info, fmt);
		}

		inline void warn(cref<string> fmt) {
			log(severity::warn, fmt);
		}

		inline void crit(cref<string> fmt) {
			log(severity::crit, fmt);
		}

		static ref<logger> instance() {
			if (!_instance) {
				_instance = mem_create<logger>();
			}

			return *_instance;
		}

		mem<sink> _sink;

		static inline mem<logger> _instance = nullptr;
	};
}

export {
	template<typename... Args>
	void LOG_INFO(cref<core::string> fmt, Args&&... args) {
		core::logger::instance().info(core::format_string(fmt, forward_data(args)...));
	}

	template<typename... Args>
	void LOG_WARN(cref<core::string> fmt, Args&&... args) {
		core::logger::instance().warn(core::format_string(fmt, forward_data(args)...));
	}

	template<typename... Args>
	void LOG_CRIT(cref<core::string> fmt, Args&&... args) {
		core::logger::instance().crit(core::format_string(fmt, forward_data(args)...));
	}
}
