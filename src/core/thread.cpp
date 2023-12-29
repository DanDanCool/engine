module;

#include "core.h"

export module core.thread;
import core.memory;

export namespace core {
	struct thread;
	typedef int (*pfn_thread)(ref<thread>, ptr<void> args);

	struct _threadargs {
		_threadargs(pfn_thread instart, ref<thread> inthread, ptr<void> inargs)
		: _start(instart), _thread(inthread), _args(inargs) {}
			_threadargs(cref<_threadargs> other);

		_threadargs(cref<_threadargs> other)
		: _start(other._start), _thread(other._thread), _args(other._args) {}

		pfn_thread _start;
		ref<thread> _thread;
		ptr<void> _args;
	};

	ref<thread> thread::operator=(cref<thread> other) {
		handle = other.handle;
		return *this;
	}

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
