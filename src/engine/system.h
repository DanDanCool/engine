#pragma once

#include <core/core.h>
#include <core/thread.h>

namespace jolly {
	struct system {
		virtual ~system() = default;

		virtual void init() = 0;
		virtual void term() = 0;

		virtual void step(f32 ms) = 0;
	};

	struct system_thread : public system {
		system_thread() = default;
		virtual ~system_thread() = default;

		virtual void init();
		virtual void term();

		void yield();
		void sleep(int ms);

		virtual void run() = 0;

		core::thread thread;
	};
}
