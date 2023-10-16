#pragma once

#include "core.h"
#include "memory.h"

namespace core {
	template <typename T>
	struct lock {
		using type = T;
		lock(cref<type> in) : _lock(in) {
			_lock.acquire();
		}

		~lock() {
			_lock.release();
		}

		cref<type> _lock;
	};

	struct mutex {
		mutex();
		mutex(mutex&& other);
		~mutex();

		ref<mutex> operator=(mutex&& other);

		bool tryacquire() const;
		void acquire() const;
		void release() const;

		ptr<void> handle;
	};

	struct semaphore {
		semaphore(u32 max = 1, u32 count = 0);
		semaphore(semaphore&& other);
		~semaphore();

		ref<semaphore> operator=(semaphore&& other);

		bool tryacquire() const;
		void acquire() const;
		void release() const;

		ptr<void> handle;
	};
}
