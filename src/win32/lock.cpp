#include <core/lock.h>

#include <windows.h>
#include <process.h>

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

	bool mutex::tryacquire() {
		DWORD res = WaitForSingleObject((HANDLE)handle.data, 0);
		return res == WAIT_TIMEOUT ? false : true;
	}

	void mutex::acquire() {
		WaitForSingleObject((HANDLE)handle.data, INFINITE);
	}

	void mutex::release() {
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

	bool semaphore::tryacquire() {
		DWORD res = WaitForSingleObject((HANDLE)handle.data, 0);
		return res == WAIT_TIMEOUT ? false : true;
	}

	void semaphore::acquire() {
		WaitForSingleObject((HANDLE)handle.data, INFINITE);
	}

	void semaphore::release() {
		ReleaseSemaphore((HANDLE)handle.data, 1, NULL);
	}
}
