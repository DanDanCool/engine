#pragma once

#include <core/table.h>

#include <vulkan/vulkan.h>

namespace jolly {
	struct vk_surface {
		vk_surface() = default;
		~vk_surface() = default;
		core::table<u32, VkSurfaceKHR> surfaces;
	};
}
