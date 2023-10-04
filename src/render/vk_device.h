#pragma once

#include <core/string.h>
#include <core/memory.h>
#include <core/tuple.h>

namespace jolly {
	struct window;
	struct vk_device {
		vk_device() = default;
		vk_device(cref<core::string> name, core::pair<u32, u32> sz = { 1280, 720 });
		~vk_device();

		void step(f32 ms);

		core::ptr<window> _window;
	};
}
