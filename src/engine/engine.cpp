#include "engine.h"

#include <core/timer.h>

namespace jolly {
	core::ptr<engine> engine::_instance = nullptr;

	engine::engine()
	: _systems(), _lock(), _run(true) {
		JOLLY_ASSERT(_instance.data == nullptr, "cannot have more than one engine instance");
	}

	engine::~engine() {
		for (auto& sys : _systems.vals()) {
			sys->term();
		}
	}

	void engine::add(cref<core::string> name, core::ptr<system> sys) {
		core::lock lock(_lock);
		auto& s = sys.get();
		_systems[name] = sys;
		s.init();
	}

	ref<system> engine::get(cref<core::string> name) {
		return _systems[name].get();
	}

	void engine::run() {
		f32 dt = 0;

		bool run = _run.get(core::memory_order_relaxed);
		while (run) {
			core::lock lock(_lock);
			core::timer timer(dt);
			for (auto& sys : _systems.vals()) {
				sys->step(dt);
			}

			run = _run.get(core::memory_order_relaxed);
		}
	}

	void engine::stop() {
		_run.set(false, core::memory_order_relaxed);
	}

	ref<engine> engine::instance() {
		if (!_instance) {
			_instance = core::ptr_create<engine>();
		}

		return *_instance;
	}
}
