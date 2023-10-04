#pragma once

#include "core.h"
#include "string.h"
#include "memory.h"

#define LOG_INFO(...) core::logger::instance().info(__VA_ARGS__)
#define LOG_WARN(...) core::logger::instance().warn(__VA_ARGS__)
#define LOG_CRIT(...) core::logger::instance().crit(__VA_ARGS__)

namespace core {
	struct sink;
	struct logger {
		logger();
		~logger();

		void _write(cref<string> info, cref<string> log);

		void info(cref<string> log);
		void warn(cref<string> log);
		void crit(cref<string> log);

		static ref<logger> instance();

		ptr<sink> _sink;

		static ptr<logger> _instance;
	};
};
