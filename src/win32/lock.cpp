#include <core/lock.h>

#include <windows.h>
#include <process.h>

namespace core {
	mutex::mutex() {
		handle = (void*)CreateMutex(NULL, false, NULL);
	}

	mutex::~mutex() {
		CloseHandle((HANDLE)handle);
		handle = nullptr;
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

	semaphore::semaphore(u32 max, u32 count) {
		handle = (void*)CreateSemaphore(NULL, count, max, NULL);
	}

	semaphore::~semaphore() {
		CloseHandle((HANDLE)handle.data);
		handle = nullptr;
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
