#pragma once

#include <core/core.h>

namespace core {
	struct _memory_order_relaxed {};
	struct _memory_order_acquire {};
	struct _memory_order_release {};

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
};
