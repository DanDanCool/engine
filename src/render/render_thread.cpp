#include "render_thread.h"

#include <core/timer.h>
#include <engine/engine.h>

#include "vk_device.h"

namespace jolly {
	render_thread::render_thread()
	: system_thread(), _device(), _run(true) {
	}

	render_thread::~render_thread() {

	}

	void render_thread::init() {
		system_thread::init();
	}

	void render_thread::term() {
		_run.set(false, core::memory_order_relaxed);

		system_thread::term();
	}

	void render_thread::run() {
		_device = core::ptr_create<vk_device>("jolly");

		f32 dt = 0;
		bool run = _run.get(core::memory_order_relaxed);

		while (run) {
			core::timer timer(dt);
			_device->step(dt);
			run = _run.get(core::memory_order_relaxed);
		}
	}

	void render_thread::step(f32 ms) {
	}
}
