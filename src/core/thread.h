#pragma once

#include "memory.h"

namespace core {
	struct thread;
	typedef int (*pfn_thread)(ref<thread>, ptr<void> args);

	struct _threadargs {
		_threadargs(pfn_thread instart, ref<thread> inthread, ptr<void> inargs);
		_threadargs(cref<_threadargs> other);

		pfn_thread _start;
		ref<thread> _thread;
		ptr<void> _args;
	};

	struct thread {
		thread() = default;
		thread(pfn_thread start, ptr<void> args);
		ref<thread> operator=(cref<thread> other);

		~thread();

		void join();
		void exit(int res = 0) const;
		void yield() const;
		void sleep(int ms) const;

		// aliases to use in lock
		void acquire();
		void release();

		ptr<void> handle;
	};
}
