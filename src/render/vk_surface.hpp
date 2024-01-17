module;

#include <core/core.h>
#include <vulkan/vulkan.h>

export module vulkan.surface;
import core.types;
import core.table;
import core.vector;
import core.lock;
import core.iterator;

constexpr u32 MAX_FRAMES_IN_FLIGHT = 2;

export namespace jolly {
	enum swapchain_state {
		none = 0,
		minimized = 1 << 0,
	};

	ENUM_CLASS_OPERATORS(swapchain_state);

	struct vk_device;
	struct vk_surface;
	struct vk_swapchain {
		vk_swapchain() = default;

		vk_swapchain(vk_swapchain&& other) {
			*this = forward_data(other);
		}

		ref<vk_swapchain> operator=(vk_swapchain&& other) {
			surface = other.surface;
			swapchain = other.swapchain;
			extent = other.extent;
			images = forward_data(other.images);
			views = forward_data(other.views);
			framebuffers = forward_data(other.framebuffers);

			for (u32 i : core::range(MAX_FRAMES_IN_FLIGHT)) {
				image_available[i] = other.image_available[i];
				render_finished[i] = other.render_finished[i];
			}

			busy = forward_data(other.busy);
			flags = other.flags;
			return *this;
		}

		~vk_swapchain() = default;

		VkSurfaceKHR surface;
		VkSwapchainKHR swapchain;
		VkExtent2D extent;
		core::vector<VkImage> images;
		core::vector<VkImageView> views;
		core::vector<VkFramebuffer> framebuffers;
		VkSemaphore image_available[MAX_FRAMES_IN_FLIGHT];
		VkSemaphore render_finished[MAX_FRAMES_IN_FLIGHT];
		core::semaphore busy;
		swapchain_state flags;
	};

	struct vk_surface {
		vk_surface(cref<vk_device> _device)
		: device(_device)
		, surfaces()
		, fences{ VK_NULL_HANDLE, VK_NULL_HANDLE }
		, format(VK_FORMAT_UNDEFINED) {}

		~vk_surface() = default;
		cref<vk_device> device;
		core::table<u32, vk_swapchain> surfaces;
		VkFence fences[MAX_FRAMES_IN_FLIGHT];
		VkFormat format;
	};
}
