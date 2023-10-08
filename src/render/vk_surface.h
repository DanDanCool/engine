#pragma once

#include <core/core.h>
#include <core/table.h>
#include <core/vector.h>

#include <vulkan/vulkan.h>

namespace jolly {
	struct vk_device;
	struct vk_swapchain {
		vk_swapchain() = default;
		~vk_swapchain() = default;
		VkSurfaceKHR surface;
		VkSwapchainKHR swapchain;
		VkExtent2D extent;
		core::vector<VkImage> images;
		core::vector<VkImageView> views;
	};

	struct vk_surface {
		vk_surface(cref<vk_device> _device);
		~vk_surface() = default;
		cref<vk_device> device;
		core::table<u32, vk_swapchain> surfaces;
		VkFormat format;
	};
}
