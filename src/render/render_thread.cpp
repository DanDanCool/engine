#include "render_thread.h"
#include <engine/engine.h>

namespace jolly {
	render_thread::render_thread() : system_thread() {

	}

	render_thread::~render_thread() {

	}

	void render_thread::init() {
		system_thread::init();
	}

	void render_thread::term() {
		system_thread::term();
	}

	void render_thread::run() {
		system_thread::sleep(5000);
		ref<engine> instance = engine::instance();
		instance.stop();
	}

	void render_thread::step(f32 ms) {

	}
}
