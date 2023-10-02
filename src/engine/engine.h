#pragma once

#include <core/table.h>
#include <core/string.h>
#include <core/memory.h>
#include <core/atom.h>
#include <core/lock.h>
#include "system.h"

namespace jolly {
	struct engine {
		engine();
		~engine();

		void add(cref<core::string> name, core::ptr<system> sys);
		ref<system> get(cref<core::string> name);

		void run();
		void stop();

		static ref<engine> instance();

		core::table<core::string, core::ptr<system>> _systems;
		core::mutex _lock;
		core::atom<bool> _run;

		static engine* _instance;
	};
}
