module;

#include <core/core.h>

export module jolly.render_thread;
import core.types;
import core.atom;
import core.memory;
import core.timer;
import core.log;
import jolly.engine;
import jolly.system;
import jolly.render_graph;
import vulkan.device;

export namespace jolly {
	struct vk_device;
	struct render_thread : public system_thread {
		render_thread()
		: system_thread(), _device(), _graph(), _run(true) {
			LOG_INFO("render system");
		}

		~render_thread() {
			LOG_INFO("~render system");
		}

		virtual void init() {
			system_thread::init();
		}

		virtual void term() {
			_run.set(false, core::memory_order_relaxed);
			system_thread::term();
		}

		virtual void run() {
			_device = core::mem_create<vk_device>("jolly");

			f32 dt = 0;
			bool run = _run.get(core::memory_order_relaxed);

			while (run) {
				core::timer timer(dt);
				_device->step(dt);
				run = _run.get(core::memory_order_relaxed);
			}
		}

		virtual void step(f32 ms) {
		}

		ref<render_graph> graph() {
			return _graph;
		}

		core::mem<vk_device> _device;
		render_graph _graph;
		core::atom<bool> _run;
	};
}
