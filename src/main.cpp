#include <core/memory.h>
#include <engine/engine.h>
#include <render/render_thread.h>

int main() {
	ref<jolly::engine> engine = jolly::engine::instance();
	engine.add("render", core::ptr_create<jolly::render_thread>().cast<jolly::system>());
	engine.run();
}
