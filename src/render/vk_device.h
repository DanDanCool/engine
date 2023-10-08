#pragma once

#include <core/string.h>
#include <core/memory.h>
#include <core/tuple.h>
#include <core/vector.h>

#include <vulkan/vulkan.h>

namespace jolly {
	struct window;

	struct vk_queue {
		VkQueue queue;
		u32 family;
	};

	struct vk_gpu {
		VkPhysicalDevice physical;
		VkDevice device;
		vk_queue graphics;
		vk_queue present;
	};

	struct vk_device {
		vk_device() = default;
		vk_device(cref<core::string> name, core::pair<u32, u32> sz = { 1280, 720 });
		~vk_device();

		void step(f32 ms);

		core::ptr<window> _window;
		VkInstance _instance;
		VkDebugUtilsMessengerEXT _debugmsg;

		core::vector<vk_gpu> _gpus;
		u32 _gpu; // gpu we use to present and render
	};
}
