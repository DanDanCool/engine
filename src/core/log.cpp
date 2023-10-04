#include "log.h"

#include "file.h"
#include "lock.h"

#include <cstdio>

namespace core {
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

	ptr<logger> logger::_instance = nullptr;

	logger::logger()
	: _sink() {
		assert(_instance == nullptr);
		_sink = ptr_create<stdout_sink>().cast<sink>();
	}

	logger::~logger() {
		_sink->flush();
	}

	void logger::_write(cref<string> info, cref<string> log) {
		_sink->write(info);

		cstr delim = ": ";
		_sink->write(memptr{ (u8*)delim, 2 });
		_sink->write(log);

		cstr newline = "\n";
		_sink->write(memptr{ (u8*)newline, 1 });
		_sink->flush();
	}

	void logger::info(cref<string> log) {
		_write("info", log);
	}

	void logger::warn(cref<string> log) {
		_write("warn", log);
	}

	void logger::crit(cref<string> log) {
		_write("crit", log);
	}

	ref<logger> logger::instance() {
		if (!_instance) {
			_instance = ptr_create<logger>();
		}

		return *_instance;
	}
}
