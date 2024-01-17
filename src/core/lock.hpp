export module core.lock;
import core.types;
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
			lock_base(cref<rwlock> in)
				: _lock(in) {}

			cref<rwlock> _lock;
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

		read_lock read() const {
			return read_lock(*this);
		}

		write_lock write() const {
			return write_lock(*this);
		}

		core::mem<void> handle;
	};

	template <typename T, typename Impl>
	struct view_base {
		using type = T;

		view_base(ref<type> in)
		: data(in) {
			Impl::acquire(get_lock(), data);
		}

		~view_base() {
			Impl::release(get_lock(), data);
		}

		auto operator->() const {
			return Impl::const_convert(data);
		}

		auto begin() const {
			return data.begin();
		}

		auto end() const {
			return data.end();
		}

		cref<core::rwlock> get_lock() const {
			return data.get_lock();
		}

		ref<type> data;
	};

	template <typename T>
	struct rview_impl {
		using type = T;
		static void acquire(cref<rwlock> l, ref<type> in) {
			l.racquire();
		}

		static void release(cref<rwlock> l, ref<type> in) {
			l.rrelease();
		}

		static const type* const_convert(ref<type> in) {
			return &static_cast<cref<type>>(in);
		}
	};

	template <typename T>
	struct wview_impl {
		using type = T;
		static void acquire(cref<rwlock> l, ref<type> in) {
			l.wacquire();
		}

		static void release(cref<rwlock> l, ref<type> in) {
			l.wrelease();
		}

		static type* const_convert(ref<type> in) {
			return &static_cast<ref<type>>(in);
		}
	};

	template<typename T>
	struct rview: public view_base<T, rview_impl<T>> {};

	template<typename T>
	struct wview: public view_base<T, wview_impl<T>> {};

	template<typename T>
	rview<T> rview_create(ref<T> in) {
		return rview<T>(in);
	}

	template<typename T>
	wview<T> wview_create(ref<T> in) {
		return wview<T>(in);
	}
}
