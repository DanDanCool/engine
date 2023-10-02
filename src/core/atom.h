#pragma once

#include "core.h"
#include <atomic>

namespace core {
	enum class memory_order {
		relaxed = std::memory_order_relaxed,
		consume = std::memory_order_consume,
		release = std::memory_order_release,
	};

	template <typename T>
	struct atom {
		using type = T;
		atom() = default;
		atom(cref<type> in) : data(in) {}

		type get(memory_order order) {
			return std::atomic_load_explicit(&data, (std::memory_order)order);
		}

		void set(type in, memory_order order) {
			std::atomic_store_explicit(&data, in, (std::memory_order)order);
		}

		bool cmpxchg(ref<type> expected, type desired, memory_order success, memory_order failure) {
			return std::atomic_compare_exchange_weak(&data, &expected, desired, (std::memory_order)success, (std::memory_order)failure);
		}

		std::atomic<T> data;
	};
}
