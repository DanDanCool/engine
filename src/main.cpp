#include <core/core.h>

import core.memory;
import jolly.engine;
import jolly.render_thread;
import jolly.ui_system;

int main() {
	ref<jolly::engine> engine = jolly::engine::instance();

	{
		auto wview = core::wview_create(engine);
		wview->add("render", core::ptr_create<jolly::render_thread>().cast<jolly::system>());
		wview->add("ui", core::ptr_create<jolly::ui_system>().cast<jolly::system>());
	}

	engine.run();
}
