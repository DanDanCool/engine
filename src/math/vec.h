#pragma once

#include <core/core.h>

namespace math {
	template <typename T>
	struct vec2 {
		using type = T;
		type x, y;
	};

	template <typename T>
	struct vec3 {
		using type = T;
		type x, y, z;
	};

	using vec2f = vec2<f32>;
	using vec3f = vec3<f32>;
	using vec2i = vec2<i32>;
	using vec3i = vec2<i32>;
};
