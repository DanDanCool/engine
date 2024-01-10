module;

#include <core/core.h>

export module math.vec;

export namespace math {
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

	template <typename T>
	struct vec4 {
		using type = T;
		type x, y, z, w;
	};

	template <typename T>
	struct mat2 {
		using type = T;
		vec2<type> x, y;
	};

	template <typename T>
	struct mat3 {
		using type = T;
		vec3<type> x, y, z;
	};

	template <typename T>
	struct mat4 {
		using type = T;
		vec4<type> x, y, z, w;
	};

	using vec2f = vec2<f32>;
	using vec3f = vec3<f32>;
	using vec2i = vec2<i32>;
	using vec3i = vec2<i32>;
	using vec4f = vec4<f32>;
	using vec4i = vec4<i32>;
};
