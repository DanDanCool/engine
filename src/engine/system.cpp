#include "system.h"

#include <core/memory.h>

namespace jolly {
	void system_thread::init() {
		auto start = [](ref<core::thread> _, core::ptr<void> args) {
			core::ptr<system_thread> sys = args.cast<system_thread>();
			sys->run();
			sys = nullptr;
			return 0;
		};

		thread = core::thread(start, core::ptr<void>((void*)this));
	}

	void system_thread::term() {
		thread.join();
	}

	void system_thread::yield() {
		thread.yield();
	}

	void system_thread::sleep(int ms) {
		thread.sleep(ms);
	}
}
