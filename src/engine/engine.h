# pragma once

#include "core/system.h"
#include "core/vector.h"

namespace jolly {
	struct engine() {
		engine();
		~engine();

		void add(cref<string> name, ref<system> sys);
		ref<system> get(cref<string> name);
		void run();

		core::table<string, system*> systems;
	};
}
