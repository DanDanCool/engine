#pragma once

#include "core/system.h"

namespace jolly {
	struct render_thread : public system {
		render_thread();
		~render_thread();

		void init();
		void term();

		void step(f32 ms);
	};
}
