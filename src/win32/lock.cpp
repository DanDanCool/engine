module;

#include <windows.h>
#include <process.h>

module core.lock;

namespace core {
	mutex::mutex()
	: handle() {
		handle = (void*)CreateMutex(NULL, false, NULL);
	}

	mutex::mutex(mutex&& other)
	: handle() {
		*this = move_data(other);
	}

	mutex::~mutex() {
		if (!handle) return;
		CloseHandle((HANDLE)handle);
		handle = nullptr;
	}

	ref<mutex> mutex::operator=(mutex&& other) {
		handle = move_data(other.handle);
		other.handle = nullptr;
		return *this;
	}

	bool mutex::tryacquire() const {
		DWORD res = WaitForSingleObject((HANDLE)handle.data, 0);
		return !(res == WAIT_TIMEOUT);
	}

	void mutex::acquire() const {
		WaitForSingleObject((HANDLE)handle.data, INFINITE);
	}

	void mutex::release() const {
		ReleaseMutex((HANDLE)handle.data);
	}

	semaphore::semaphore(u32 max, u32 count)
	: handle() {
		handle = (void*)CreateSemaphore(NULL, count, max, NULL);
	}

	semaphore::semaphore(semaphore&& other)
	: handle() {
		*this = move_data(other);
	}

	semaphore::~semaphore() {
		if (!handle) return;
		CloseHandle((HANDLE)handle.data);
		handle = nullptr;
	}

	ref<semaphore> semaphore::operator=(semaphore&& other) {
		handle = move_data(other.handle);
		other.handle = nullptr;
		return *this;
	}

	bool semaphore::tryacquire() const {
		DWORD res = WaitForSingleObject((HANDLE)handle.data, 0);
		return !(res == WAIT_TIMEOUT);
	}

	void semaphore::acquire() const {
		WaitForSingleObject((HANDLE)handle.data, INFINITE);
	}

	void semaphore::release() const {
		ReleaseSemaphore((HANDLE)handle.data, 1, NULL);
	}

	rwlock::rwlock()
	: handle() {
		SRWLock* lock = (SRWLock*)alloc8((u32)sizeof(SRWLock));
		InitializeSRWLock(lock);
		handle = (void*)lock;
	}

	rwlock::rwlock(rwlock&& other)
	: handle() {
		*this = forward_data(other);
	}

	rwlock::~rwlock() {
		if (!handle) return;
		free8(handle.data);
	}

	ref<rwlock> rwlock::operator=(rwlock&& other) {
		handle = forward_data(other.handle);
		other.handle = nullptr;
		return *this;
	}

	bool rwlock::tryracquire() const {
		return TryAcquireSWRLockShared((SRWLock*)handle.data);
	}

	bool rwlock::trywacquire() const {
		return TryAcquireSWRLockExclusive((SRWLock*)handle.data);
	}

	void rwlock::racquire() const {
		AcquireSRWLockShared((SRWLock*)handle.data);
	}

	void rwlock::rrlease() const {
		ReleaseSWRLockShared((SRWLock*)handle.data);
	}

	void rwlock::wacquire() const {
		AcquireSRWLockExclusive((SRWLock*)handle.data);
	}

	void rwlock::wrelease() const {
		ReleaseSRWLockExclusive((SWRLock*)handle.data);
	}

	bool rwlock::tryacquire() const {
		return trywacquire();
	}

	void rwlock::acquire() const {
		wacquire();
	}

	void rwlock::release() const {
		wrelease();
	}
}
