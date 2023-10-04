#include "vk_device.h"

#include <core/log.h>
#include <win32/window.h>

#include <vulkan/vulkan.h>

namespace jolly {
	vk_device::vk_device(cref<core::string> name, core::pair<u32, u32> sz)
	: _window() {
		u32 extensions;
		vkEnumerateInstanceExtensionProperties(NULL, &extensions, NULL);
		LOG_INFO("vkextension: %", extensions);
		_window = core::ptr_create<window>(name, sz);
	}

	vk_device::~vk_device() {

	}

	void vk_device::step(f32 ms) {
		_window->step(ms);
	}
}
