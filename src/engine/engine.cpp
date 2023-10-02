#include "engine.h"

#include <core/timer.h>

namespace jolly {
	engine* engine::_instance = nullptr;

	engine::engine()
	: _systems(), _lock(), _run(true) {
		assert(_instance == nullptr);
		_instance = this;
	}

	engine::~engine() {
		for (auto& sys : _systems.vals()) {
			sys->term();
		}
	}

	void engine::add(cref<core::string> name, core::ptr<system> sys) {
		core::lock lock(_lock);
		ref<system> s = sys.ref();
		_systems[name] = sys;
		s.init();
	}

	ref<system> engine::get(cref<core::string> name) {
		return _systems[name].ref();
	}

	void engine::run() {
		f32 dt = 0;

		bool run = _run.get(core::memory_order::relaxed);
		while (run) {
			core::lock lock(_lock);
			core::timer timer(dt);
			for (auto& sys : _systems.vals()) {
				sys->step(dt);
			}

			run = _run.get(core::memory_order::relaxed);
		}
	}

	void engine::stop() {
		_run.set(false, core::memory_order::relaxed);
	}

	ref<engine> engine::instance() {
		return *_instance;
	}
}
