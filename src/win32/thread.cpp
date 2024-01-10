module;

#include <core/core.h>
#include <windows.h>
#include <process.h>

module core.thread;

namespace core {
	struct _threadargs {
		_threadargs(pfn_thread instart, ref<thread> inthread, ptr<void>&& inargs)
		: _start(instart), _thread(inthread), _args(forward_data(inargs)) {}

		_threadargs(_threadargs&& other)
		: _start(other._start), _thread(other._thread), _args(forward_data(other._args)) {}

		pfn_thread _start;
		ref<thread> _thread;
		ptr<void> _args;
	};

	static int __stdcall start_wrapper(void* in) {
		ptr<_threadargs> args = ptr<void>(in).cast<_threadargs>();
		return args->_start(args->_thread, forward_data(args->_args));
	}

	thread::thread(pfn_thread start, ptr<void>&& args)
	: handle() {
		ptr<_threadargs> args_wrapper = ptr_create<_threadargs>(_threadargs{start, *this, forward_data(args)});
		handle = (void*)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)start_wrapper,
				(void*)args_wrapper.data, 0, NULL);
		args_wrapper = nullptr;
	}

	thread::~thread() {
		if (!handle) return;
		join();
	}

	void thread::join() {
		WaitForSingleObject((HANDLE)handle.data(), INFINITE);
		handle = nullptr;
	}

	void thread::exit(int res) const {
		_endthreadex(res);
	}

	void thread::yield() const {
		(void)SwitchToThread();
	}

	void thread::sleep(int ms) const {
		Sleep(ms);
	}
}
