#pragma once

#include "core.h"

namespace jolly {
	struct system {
		system() = delete;

		virtual void init();
		virtual void term();

		virtual void step(f32 ms);
	};
}
