#include "render_thread.h"
#include <engine/engine.h>

#include <core/timer.h>

namespace jolly {
	render_thread::render_thread()
	: system_thread(), _window(), _run(true) {
	}

	render_thread::~render_thread() {

	}

	void render_thread::init() {
		system_thread::init();
	}

	void render_thread::term() {
		_run.set(false, core::memory_order::relaxed);

		system_thread::term();
	}

	void render_thread::run() {
		_window = core::ptr_create<window>("jolly");

		core::pair<u32, u32> size = { 200, 200 };
		for (int i : core::range(5)) {
			_window->create("foo", size);
		}

		f32 dt = 0;
		bool run = _run.get(core::memory_order::relaxed);

		while (run) {
			core::timer timer(dt);
			_window->step(dt);
			run = _run.get(core::memory_order::relaxed);
		}
	}

	void render_thread::step(f32 ms) {
	}
}
