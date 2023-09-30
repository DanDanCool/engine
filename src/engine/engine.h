# pragma once

#include "core/system.h"
#include "core/table.h"
#include "core/string.h"

namespace jolly {
	struct engine {
		engine();
		~engine();

		void add(cref<string> name, ptr<system> sys);
		ref<system> get(cref<string> name);
		void run();

		core::table<string, ptr<system>> _systems;
	};
}
