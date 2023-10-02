#include "scheduler.h"

#include "core.h"
#include "thread.h"

namespace core {
	_threadargs::_threadargs(pfn_thread instart, ref<thread> inthread, ptr<void> inargs)
	: _start(instart), _thread(inthread), _args(inargs) {}
	_threadargs::_threadargs(cref<_threadargs> other)
	: _start(other._start), _thread(other._thread), _args(other._args) {}

	ref<thread> thread::operator=(cref<thread> other) {
		handle = other.handle;
		return *this;
	}
}
