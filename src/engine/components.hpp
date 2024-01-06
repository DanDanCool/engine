module;

#include <core/core.h>

export module engine.components;
import math.vec;

namespace jolly {
	struct quad_component {
		math::vec2f pos;
		math::vec2f scale;
		math::vec3f color;
	};
}
