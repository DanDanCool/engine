#pragma once

#include <engine/system.h>

#include <core/core.h>
#include <core/atom.h>
#include <core/memory.h>

namespace jolly {
	struct vk_device;
	struct render_thread : public system_thread {
		render_thread();
		~render_thread();

		virtual void init();
		virtual void term();

		virtual void run();
		virtual void step(f32 ms);

		core::ptr<vk_device> _device;
		core::atom<bool> _run;
	};
}
