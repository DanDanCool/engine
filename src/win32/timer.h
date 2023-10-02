#pragma once

#include <core/core.h>

namespace core {
	struct timer {
		timer(ref<f32> res);
		~timer();

		i64 start;
		ref<f32> result;

		static i64 frequency;
	};
}
