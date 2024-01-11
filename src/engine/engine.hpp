module;

#include <core/core.h>

export module jolly.engine;
import jolly.system;
import jolly.ecs;
import core.table;
import core.string;
import core.atom;
import core.lock;
import core.timer;
import core.memory;
import core.log;

export namespace jolly {
	struct engine {
		engine()
		: _systems()
		, _ecs()
		, _busy()
		, _run(true) {
			JOLLY_ASSERT(_instance.data == nullptr, "cannot have more than one engine instance");
		}

		~engine() {
		}

		// obtain a rview/wview
		void add(cref<core::string> name, core::ptr<system>&& sys) {
			auto& s = sys.get();
			_systems[name] = forward_data(sys);
			s.init();
		}

		ref<system> get(cref<core::string> name) const {
			return _systems[name].get();
		}

		// do not call with an owning view
		void run() {
			f32 dt = 0;

			bool run = _run.get(core::memory_order_relaxed);
			while (run) {
				core::lock lock(_busy.read());
				core::timer timer(dt);
				for (auto& sys : _systems.vals()) {
					sys->step(dt);
				}

				run = _run.get(core::memory_order_relaxed);
			}

			for (auto& sys : _systems.vals()) {
				sys->term();
			}
		}

		void stop() {
			_run.set(false, core::memory_order_relaxed);
		}

		ref<ecs> get_ecs() {
			return _ecs;
		}

		cref<core::rwlock> get_lock() const {
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

		static inline core::ptr<engine> _instance = nullptr;
	};
}
