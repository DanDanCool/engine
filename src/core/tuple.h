#pragma once

#include "core.h"
#include "memory.h"

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

	template<typename T, u32 N>
	struct array {
		static constexpr u32 size = N;
		using type = T;

		array() : data(), index(0) {
			zero8((u8*)data, sizeof(data));
		}

		void add(cref<type> val) {
			data[index++] = val;
		}

		ref<type> operator[](u32 idx) {
			return data[idx];
		}

		type data[size];
		u32 index;
	};
};
