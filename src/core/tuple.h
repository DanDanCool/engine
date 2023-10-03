#pragma once

#include "core.h"

namespace core {
	template <typename T1, typename T2>
	struct pair {
		pair() = default;
		pair(cref<T1> a, cref<T2> b)
			: one(a), two(b) {}

		~pair() = default;

		T1 one;
		T2 two;
	};
};
