#include <core/memory.h>
#include <engine/engine.h>
#include <render/render_thread.h>

int main() {
	core::ptr<jolly::engine> engine = core::ptr_create<jolly::engine>();
	engine->add("render", core::ptr_create<jolly::render_thread>().cast<jolly::system>());
	engine->run();
}
