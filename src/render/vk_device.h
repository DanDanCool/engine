#pragma once

#include <core/string.h>
#include <core/memory.h>
#include <core/tuple.h>
#include <core/vector.h>
#include <math/vec.h>

#include <vulkan/vulkan.h>

namespace jolly {
	constexpr u32 MAX_FRAMES_IN_FLIGHT = 2;

	enum class vertex_data {
		vec2,
		vec3,
	};

	enum class vertex_input {
		vertex,
		instance
	};

	typedef core::pair<vertex_data, u32> vertex_attribute;
	typedef core::pair<core::vector<vertex_attribute>, vertex_input> binding_description;

	struct window;

	struct vk_queue {
		VkQueue queue;
		u32 family;
	};

	struct vk_gpu {
		VkPhysicalDevice physical;
		VkDevice device;
		vk_queue graphics;
		vk_queue compute;
		vk_queue transfer;
		vk_queue present;
	};

	struct vk_pipeline {
		VkRenderPass renderpass;
		VkPipelineLayout layout;
		VkPipeline pipeline;

		VkBuffer buffer;
		VkDeviceMemory memory;

		VkBuffer index;
		VkDeviceMemory index_memory;

		u32 bindings;
	};

	struct vk_cmdpool {
		vk_cmdpool() = default;
		vk_cmdpool(cref<vk_gpu> gpu, u32 family);
		void destroy(cref<vk_gpu> gpu); // needs explicit destruction

		VkCommandBuffer create(cref<vk_gpu> gpu);
		void destroy(VkCommandBuffer buffer, VkFence fence);

		void gc(cref<vk_gpu> gpu);

		VkCommandPool pool;
		core::vector<VkCommandBuffer> free;
		core::vector<core::pair<VkCommandBuffer, VkFence>> busy;
	};

	struct vk_device {
		vk_device() = default;
		vk_device(cref<core::string> name, math::vec2i sz = { 1280, 720 });
		~vk_device();

		void pipeline(cref<core::string> fname, cref<core::vector<binding_description>> layout);

		ref<vk_gpu> main_gpu() const;

		void step(f32 ms);

		core::ptr<window> _window;
		VkInstance _instance;
		VkDebugUtilsMessengerEXT _debugmsg;

		vk_pipeline _pipeline;
		vk_cmdpool _cmdpool;

		core::vector<vk_gpu> _gpus;
		u32 _gpu; // gpu we use to present and render
		u32 _frame;
	};
}
