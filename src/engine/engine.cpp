#include "engine.h"

namespace jolly {
	engine::engine()
	: _systems() {

	}

	engine::~engine() {
		for (cref<ptr<system>> sys : _systems.vals()) {
			sys->term();
		}
	}

	void engine::add(cref<string> name, ptr<system> sys) {
		ref<system> s = sys.ref();
		_systems[name] = sys;
		s.init();
	}

	ref<system> engine::get(cref<string> name) {
		return _systems[name].ref();
	}

	void engine::run() {
		while (true) {
			for (cref<ptr<system>> sys : _systems.vals()) {
				sys->step(dt);
			}
		}
	}
}
