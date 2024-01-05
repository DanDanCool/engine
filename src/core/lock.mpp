module;

#include "core.h"

export module core.lock;
import core.memory;
import core.atom;

export namespace core {
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

		core::handle handle;
	};

	struct semaphore {
		semaphore(u32 max = 1, u32 count = 0);
		semaphore(semaphore&& other);
		~semaphore();

		ref<semaphore> operator=(semaphore&& other);

		bool tryacquire() const;
		void acquire() const;
		void release() const;

		core::handle handle;
	};

	// writer priority rw lock
	struct rwlock {
		rwlock();
		rwlock(rwlock&& other);
		~rwlock();

		ref<rwlock> operator=(rwlock&& other);

		bool tryracquire() const;
		bool trywacquire() const;
		void racquire() const;
		void rrelease() const;
		void wacquire() const;
		void wrelease() const;

		struct lock_base {
			lock_base(ref<rwlock> in)
				: _lock(in) {}

			ref<rwlock> _lock;
		};

		struct read_lock: public lock_base {
			using lock_base::lock_base;

			bool tryacquire() const {
				return _lock.tryracquire();
			}

			void acquire() const {
				_lock.racquire();
			}

			void release() const {
				_lock.rrelease();
			}
		};

		struct write_lock: public lock_base {
			using lock_base::lock_base;

			bool tryacquire() const {
				return _lock.trywacquire();
			}

			void acquire() const {
				_lock.wacquire();
			}

			void release() const {
				_lock.wrelease();
			}
		};

		auto read() const {
			return read_lock(*this);
		}

		auto write() const {
			return write_lock(*this);
		}

		core::ptr<void> handle;
	};

	template <typename T, typename Impl>
	struct view_base {
		using type = T;

		view_base(ref<type> in)
		: data(in) {
			Impl::acquire(lock());
		}

		~view_base() {
			Impl::release(lock());
		}

		auto operator->() const {
			return &Impl::const_convert(data);
		}

		ref<core::rwlock> lock() {
			return data.get_lock();
		}

		ref<type> data;
	};

	struct rview_impl {
		static void acquire(ref<core::rwlock> l) {
			l.racquire();
		}

		static void release(ref<core::rwlock> l) {
			l.rrelease();
		}

		template<typename T>
		cref<T> const_convert(ref<T> in) {
			return static_cast<cref<T>>(in);
		}
	};

	struct wview_impl {
		static void acquire(ref<core::rwlock> l) {
			l.wacquire();
		}

		static void release(ref<core::rwlock> l) {
			l.wrelease();
		}

		template<typename T>
		ref<T> const_convert(ref<T> in) {
			return static_cast<ref<T>>(in);
		}
	};

	template<typename T>
	using rview = view_base<T, rview_impl>;

	template<typename T>
	using wview = view_base<T, wview_impl>;
}
