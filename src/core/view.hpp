module;

#include <core/core.h>

export module core.view;

namespace core {
export namespace view {
	template <typename T>
	auto find(cref<T> v, auto fn) {
		for (auto it = v.begin(); it != v.end(); it++) {
			if (fn(*it)) {
				return it;
			}
		}

		return v.end();
	}

	template <typename T>
	bool has(cref<T> v, auto fn) {
		for (auto x : v) {
			if (fn(x)) {
				return true;
			}
		}

		return false;
	}
}
}
