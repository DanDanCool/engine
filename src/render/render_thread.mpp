module;

#include <core/core.h>

export module jolly.render_thread;
import core.atom;
import core.memory;
import core.timer;
import core.log;
import jolly.engine;
import jolly.system;
import jolly.render_graph;
import vulkan.device;

namespace jolly {
	struct vk_device;
	struct render_thread : public system_thread {
		render_thread();
		~render_thread();

		virtual void init();
		virtual void term();

		virtual void run();
		virtual void step(f32 ms);

		render_thread()
		: system_thread(), _device(), _run(true) {
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
			_device = core::ptr_create<vk_device>("jolly");

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

		ref<render_graph> graph();

		core::ptr<vk_device> _device;
		core::atom<bool> _run;
	};
}
