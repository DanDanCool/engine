module;

#include <core/core.h>
#include <windows.h>
#include <process.h>
#include <synchapi.h>

module core.lock;

namespace core {
	mutex::mutex()
	: handle() {
		handle = (ptr<void>)CreateMutex(NULL, false, NULL);
	}

	mutex::mutex(fwd<mutex> other)
	: handle() {
		*this = forward_data(other);
	}

	mutex::~mutex() {
		if (!handle) return;
		CloseHandle((HANDLE)handle.data());
	}

	ref<mutex> mutex::operator=(fwd<mutex> other) {
		handle = forward_data(other.handle);
		other.handle = nullptr;
		return *this;
	}

	bool mutex::tryacquire() const {
		DWORD res = WaitForSingleObject((HANDLE)handle.data(), 0);
		return !(res == WAIT_TIMEOUT);
	}

	void mutex::acquire() const {
		WaitForSingleObject((HANDLE)handle.data(), INFINITE);
	}

	void mutex::release() const {
		ReleaseMutex((HANDLE)handle.data());
	}

	semaphore::semaphore(u32 max, u32 count)
	: handle() {
		handle = (ptr<void>)CreateSemaphore(NULL, count, max, NULL);
	}

	semaphore::semaphore(fwd<semaphore> other)
	: handle() {
		*this = forward_data(other);
	}

	semaphore::~semaphore() {
		if (!handle) return;
		CloseHandle((HANDLE)handle.data());
	}

	ref<semaphore> semaphore::operator=(semaphore&& other) {
		handle = forward_data(other.handle);
		other.handle = nullptr;
		return *this;
	}

	bool semaphore::tryacquire() const {
		DWORD res = WaitForSingleObject((HANDLE)handle.data(), 0);
		return !(res == WAIT_TIMEOUT);
	}

	void semaphore::acquire() const {
		WaitForSingleObject((HANDLE)handle.data(), INFINITE);
	}

	void semaphore::release() const {
		ReleaseSemaphore((HANDLE)handle.data(), 1, NULL);
	}

	rwlock::rwlock()
	: handle() {
		ptr<SRWLOCK> lock = (ptr<SRWLOCK>)alloc8((u32)sizeof(SRWLOCK)).data;
		InitializeSRWLock(lock);
		handle = (ptr<void>)lock;
	}

	rwlock::rwlock(fwd<rwlock> other)
	: handle() {
		*this = forward_data(other);
	}

	rwlock::~rwlock() {
		// SRWLOCK can be safely deleted as long as there are no acquires
	}

	ref<rwlock> rwlock::operator=(fwd<rwlock> other) {
		handle = forward_data(other.handle);
		other.handle = nullptr;
		return *this;
	}

	bool rwlock::tryracquire() const {
		return TryAcquireSRWLockShared((ptr<SRWLOCK>)handle.data);
	}

	bool rwlock::trywacquire() const {
		return TryAcquireSRWLockExclusive((ptr<SRWLOCK>)handle.data);
	}

	void rwlock::racquire() const {
		AcquireSRWLockShared((ptr<SRWLOCK>)handle.data);
	}

	void rwlock::rrelease() const {
		ReleaseSRWLockShared((ptr<SRWLOCK>)handle.data);
	}

	void rwlock::wacquire() const {
		AcquireSRWLockExclusive((ptr<SRWLOCK>)handle.data);
	}

	void rwlock::wrelease() const {
		ReleaseSRWLockExclusive((ptr<SRWLOCK>)handle.data);
	}
}
