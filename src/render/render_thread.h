#pragma once

#include <engine/system.h>

namespace jolly {
	struct render_thread : public system_thread {
		render_thread();
		~render_thread();

		virtual void init();
		virtual void term();

		virtual void run();
		virtual void step(f32 ms);
	};
}
