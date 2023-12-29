module;

#include "core.h"

export module core.atom;

#define ATOM_DEFINE_GET(order) \
type get(order) const { \
	return atomic_load<type, order>(*this); \
}

#define ATOM_DEFINE_SET(order) \
void set(type data, order) { \
	atomic_store<type, order>(*this, data); \
}

#define ATOM_DEFINE_ADD(order) \
void add(type data, order) { \
	atomic_add<type, order>(*this, data); \
}

#define ATOM_DEFINE_SUB(order) \
void add(type data, order) { \
	atomic_sub<type, order>(*this, data); \
}

#define ATOM_DEFINE_CMPXCHG(success, failure) \
bool cmpxchg(ref<type> expected, type desired, success, failure) { \
	return atomic_cmpxchg<type, success, failure>(*this, expected, desired); \
}

export namespace core {
	struct _memory_order_relaxed {};
	struct _memory_order_acquire {};
	struct _memory_order_release {};

	constexpr auto memory_order_relaxed = _memory_order_relaxed{};
	constexpr auto memory_order_release = _memory_order_release{};
	constexpr auto memory_order_acquire = _memory_order_acquire{};

	template <typename T>
	struct atom_base {
		using type = T;

		// constructors are not atomic
		atom_base() = default;
		atom_base(type in) : data(in) {}

		type data;
	};

	template<typename T, typename order>
	T atomic_load(cref<atom_base<T>> obj);

	template<typename T, typename order>
	void atomic_store(ref<atom_base<T>> obj, T data);

	template<typename T, typename success, typename failure>
	bool atomic_cmpxchg(ref<atom_base<T>> obj, ref<T> expected, T desired);

	template <typename T>
	struct atom : public atom_base<T> {
		using type = T;
		using atom_base::atom_base;

		template<typename order>
		type get(order) const {
			static_assert(false);
			return atom_base::data;
		}

		ATOM_DEFINE_GET(_memory_order_relaxed);
		ATOM_DEFINE_GET(_memory_order_acquire);

		template<typename order>
		void set(type data, order) {
			static_assert(false);
		}

		ATOM_DEFINE_SET(_memory_order_relaxed);
		ATOM_DEFINE_SET(_memory_order_release);

		template <typename order>
		void add(type arg, order) {
			static_assert(false);
		}

		ATOM_DEFINE_ADD(_memory_order_relaxed);
		ATOM_DEFINE_ADD(_memory_order_release);

		template <typename order>
		void sub(type arg, order) {
			static_assert(false);
		}

		ATOM_DEFINE_SUB(_memory_order_relaxed);
		ATOM_DEFINE_SUB(_memory_order_release);

		template<typename success, typename failure>
		bool cmpxchg(ref<type> expected, type desired, success, failure) {
			static_assert(false);
			return false;
		}

		ATOM_DEFINE_CMPXCHG(_memory_order_relaxed, _memory_order_relaxed);
		ATOM_DEFINE_CMPXCHG(_memory_order_relaxed, _memory_order_release);
		ATOM_DEFINE_CMPXCHG(_memory_order_release, _memory_order_relaxed);
		ATOM_DEFINE_CMPXCHG(_memory_order_release, _memory_order_release);
	};
}
