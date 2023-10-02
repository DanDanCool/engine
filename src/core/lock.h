#pragma once

#include "core.h"
#include "memory.h"

namespace core {
	template <typename T>
	struct lock {
		using type = T;
		lock(ref<type> in) : _lock(in) {
			_lock.acquire();
		}

		~lock() {
			_lock.release();
		}

		ref<type> _lock;
	};

	struct mutex {
		mutex();
		~mutex();

		bool tryacquire();
		void acquire();
		void release();

		ptr<void> handle;
	};

	struct semaphore {
		semaphore(u32 max = 1, u32 count = 0);
		~semaphore();

		bool tryacquire();
		void acquire();
		void release();

		ptr<void> handle;
	};
}
