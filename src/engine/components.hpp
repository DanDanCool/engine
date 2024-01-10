module;

#include <core/core.h>

export module jolly.components;
import math.vec;

export namespace jolly {
	struct quad_component {
		math::vec2f pos;
		math::vec2f scale;
		math::vec3f color;
	};
}
