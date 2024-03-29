module;

#include "core.h"

export module core.thread;
import core.types;
import core.memory;

export namespace core {
	struct thread;
	typedef int (*pfn_thread)(ref<thread>, mem<void>&& args);

	struct thread {
		thread() = default;
		thread(pfn_thread start, mem<void>&& args);

		ref<thread> operator=(thread&& other) {
			handle = forward_data(other.handle);
			other.handle = nullptr;
			return *this;
		}

		~thread();

		void join();
		void exit(int res = 0) const;
		void yield() const;
		void sleep(int ms) const;

		// aliases to use in lock
		void acquire() {
			join();
		}

		void release() {
			// do nothing
		}

		core::handle handle;
	};
}
