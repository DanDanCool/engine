module;

#include <core/core.h>
#include <windows.h>
#include <process.h>

module core.thread;

namespace core {
	static int __stdcall start_wrapper(void* in) {
		ptr<_threadargs> args = ptr<void>(in).cast<_threadargs>();
		return args->_start(args->_thread, args->_args);
	}

	thread::thread(pfn_thread start, ptr<void> args)
	: handle() {
		ptr<_threadargs> args_wrapper = ptr_create<_threadargs>(_threadargs{start, *this, args});
		handle = (void*)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)start_wrapper,
				(void*)args_wrapper.data, 0, NULL);
		args_wrapper = nullptr;
	}

	thread::~thread() {
		if (!handle) return;
		join();
	}

	void thread::join() {
		WaitForSingleObject((HANDLE)handle.data, INFINITE);
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

	void thread::acquire() {
		join();
	}

	void thread::release() {
		// do nothing
	}
}
