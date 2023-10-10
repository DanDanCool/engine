#pragma once

#include <core/core.h>
#include <core/table.h>
#include <core/vector.h>

#include <vulkan/vulkan.h>

#include "vk_device.h"

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
		core::vector<VkFramebuffer> framebuffers;
		VkSemaphore image_available[MAX_FRAMES_IN_FLIGHT];
		VkSemaphore render_finished[MAX_FRAMES_IN_FLIGHT];
	};

	struct vk_surface {
		vk_surface(cref<vk_device> _device);
		~vk_surface() = default;
		cref<vk_device> device;
		core::table<u32, vk_swapchain> surfaces;
		VkFence fence[MAX_FRAMES_IN_FLIGHT];
		VkFormat format;
	};
}
