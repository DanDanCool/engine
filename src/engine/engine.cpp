module;

#include <core/core.h>

export module jolly.engine;
import jolly.system;
import core.table;
import core.string;
import core.atom;
import core.lock;
import core.timer;
import core.ecs;

namespace jolly {
	struct engine {
		engine()
		: _systems()
		, _ecs()
		, _busy()
		, _run(true) {
			JOLLY_ASSERT(_instance.data == nullptr, "cannot have more than one engine instance");
		}

		~engine() {
			for (auto& sys : _systems.vals()) {
				sys->term();
			}
		}

		void add(cref<core::string> name, core::ptr<system>&& sys) {
			core::lock lock(_busy);
			auto& s = sys.get();
			_systems[name] = forward_data(sys);
			s.init();
		}

		ref<system> engine::get(cref<core::string> name) const {
			return _systems[name].get();
		}

		void run();

		void engine::run() {
			f32 dt = 0;

			bool run = _run.get(core::memory_order_relaxed);
			while (run) {
				core::lock lock(_busy);
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

		ref<ecs> engine::get_ecs() const {
			return _ecs;
		}

		ref<core::rwlock> get_lock() const {
			return _busy;
		}

		static ref<engine> instance() {
			if (!_instance) {
				_instance = core::ptr_create<engine>();
			}

			return *_instance;
		}

		core::table<core::string, core::ptr<system>> _systems;
		ecs _ecs;
		core::rwlock _busy;
		core::atom<bool> _run;

		static core::ptr<engine> _instance = nullptr;
	};
}
