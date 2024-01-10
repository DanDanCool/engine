module;

#include "core.h"

export module core.timer;

export namespace core {
	struct timer {
		timer(ref<f32> res);
		~timer();

		i64 start;
		ref<f32> result;

		static inline i64 frequency = -1;
	};
}
