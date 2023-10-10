#include "vk_device.h"
#include "vk_surface.h"

#include <core/log.h>
#include <core/set.h>
#include <core/file.h>
#include <win32/window.h>

namespace jolly {
	static VKAPI_ATTR VkBool32 VKAPI_CALL vkdebugcb(
		VkDebugUtilsMessageSeverityFlagBitsEXT severity,
		VkDebugUtilsMessageTypeFlagsEXT type,
		const VkDebugUtilsMessengerCallbackDataEXT* cbdata,
		void* userdata) {
		LOG_INFO("vulkan: %", cbdata->pMessage);

		return VK_FALSE;
	}

	vk_device::vk_device(cref<core::string> name, core::pair<u32, u32> sz)
	: _window()
	, _instance(VK_NULL_HANDLE)
	, _debugmsg(VK_NULL_HANDLE)
	, _pipeline{VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE}
	, _cmdpool()
	, _gpus(0)
	, _gpu(U32_MAX)
	, _frame(0) {
		_window = core::ptr_create<window>(name, sz);
		_window->create("test", { 480, 480 });

		// enable validation layers
		core::vector<cstr> enabled_layers = {
			"VK_LAYER_KHRONOS_validation"
		};

		u32 count = 0;
		vkEnumerateInstanceLayerProperties(&count, nullptr);
		core::vector<VkLayerProperties> layers(count);
		vkEnumerateInstanceLayerProperties(&count, layers.data);
		layers.size = count;

		for (auto name : enabled_layers) {
			bool found = false;
			for (auto& layer : layers) {
				if (core::cmpeq(name, layer.layerName)) {
					found = true;
					break;
				}
			}

			assert(found);
		}

		VkApplicationInfo app_info{};
		app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.pApplicationName = "jolly";
		app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		app_info.pEngineName = "jolly_engine";
		app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		app_info.apiVersion = VK_API_VERSION_1_0;

		// enable extensions
		count = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
		core::vector<VkExtensionProperties> extensions(count);
		vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data);
		extensions.size = count;

		LOG_INFO("vkextensions");
		for (auto& extension : extensions) {
			LOG_INFO("extension: %", extension.extensionName);
		}

		core::vector<cstr> enabled_extensions = _window->vk_extensions(extensions);
		enabled_extensions.add(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

		VkDebugUtilsMessengerCreateInfoEXT msg_info{};
		msg_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		msg_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		msg_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		msg_info.pfnUserCallback = vkdebugcb;
		msg_info.pUserData = nullptr;

		// create instance
		VkInstanceCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		info.pApplicationInfo = &app_info;
		info.enabledExtensionCount = enabled_extensions.size;
		info.ppEnabledExtensionNames = enabled_extensions.data;
		info.enabledLayerCount = enabled_layers.size;
		info.ppEnabledLayerNames = enabled_layers.data;
		info.pNext = &msg_info;

		vkCreateInstance(&info, nullptr, &_instance);

		// create debug messenger
		auto fn = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_instance, "vkCreateDebugUtilsMessengerEXT");
		fn(_instance, &msg_info, nullptr, &_debugmsg);

		_window->vk_init(*this);

		// create gpus
		count = 0;
		vkEnumeratePhysicalDevices(_instance, &count, nullptr);
		core::vector<VkPhysicalDevice> devices(count);
		vkEnumeratePhysicalDevices(_instance, &count, devices.data);
		devices.size = count;

		auto device_suitable = [](ref<vk_gpu> gpu, VkSurfaceKHR surface) {
			VkPhysicalDeviceProperties props{};
			vkGetPhysicalDeviceProperties(gpu.physical, &props);

			VkPhysicalDeviceFeatures features{};
			vkGetPhysicalDeviceFeatures(gpu.physical, &features);

			// queue info
			u32 count = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(gpu.physical, &count, nullptr);
			core::vector<VkQueueFamilyProperties> queues(count);
			vkGetPhysicalDeviceQueueFamilyProperties(gpu.physical, &count, queues.data);
			queues.size = count;

			u32 graphics = U32_MAX;
			u32 present = U32_MAX;

			for (int i : core::range(queues.size)) {
				auto& queue = queues[i];

				VkBool32 present_support = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(gpu.physical, i, surface, &present_support);
				if (present_support) {
					present = i;
				}

				if (queue.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
					graphics = i;
				}
			}

			core::set<u32> indices = { graphics, present };
			core::vector<VkDeviceQueueCreateInfo> queue_info(indices.size);
			for (u32 i : indices) {
				if (i == U32_MAX) continue;
				float priority = 1;
				auto& info = queue_info.add();
				info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				info.queueFamilyIndex = i;
				info.queueCount = 1;
				info.pQueuePriorities = &priority;
			}

			// check for extensions
			core::vector<cstr> extensions = {
				VK_KHR_SWAPCHAIN_EXTENSION_NAME
			};

			count = 0;
			vkEnumerateDeviceExtensionProperties(gpu.physical, nullptr, &count, nullptr);
			core::vector<VkExtensionProperties> available(count);
			vkEnumerateDeviceExtensionProperties(gpu.physical, nullptr, &count, available.data);
			available.size = count;

			bool all_extensions = true;
			for (cstr name : extensions) {
				bool found = false;
				for (auto& extension : available) {
					if (core::cmpeq(name, extension.extensionName)) {
						found = true;
						break;
					}
				}

				all_extensions &= found;
				if (!all_extensions) break;
			}

			// check swapchain support
			bool swapchain = true;
			count = 0;
			vkGetPhysicalDeviceSurfaceFormatsKHR(gpu.physical, surface, &count, nullptr);
			swapchain = swapchain && count != 0;
			count = 0;
			vkGetPhysicalDeviceSurfacePresentModesKHR(gpu.physical, surface, &count, nullptr);
			swapchain = swapchain && count != 0;

			// create logical device
			VkPhysicalDeviceFeatures enabled{};
			VkDeviceCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			info.pQueueCreateInfos = queue_info.data;
			info.queueCreateInfoCount = queue_info.size;
			info.pEnabledFeatures = &enabled;
			if (all_extensions) {
				info.ppEnabledExtensionNames = extensions.data;
				info.enabledExtensionCount = extensions.size;
			}
			vkCreateDevice(gpu.physical, &info, nullptr, &gpu.device);

			if (graphics != U32_MAX) {
				vkGetDeviceQueue(gpu.device, graphics, 0, &gpu.graphics.queue);
				gpu.graphics.family = graphics;
			}

			if (present != U32_MAX) {
				vkGetDeviceQueue(gpu.device, present, 0, &gpu.present.queue);
				gpu.present.family = present;
			}

			// decide if this device is suitable
			bool all_queues = graphics != U32_MAX && present != U32_MAX;
			bool discrete = props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
			return discrete && all_queues && all_extensions && swapchain;
		};

		VkSurfaceKHR surface = _window->vk_main_swapchain().surface;
		for (auto device : devices) {
			ref<vk_gpu> gpu = _gpus.add();
			gpu.physical = device;
			if (device_suitable(gpu, surface) && _gpu == U32_MAX) {
				_gpu = _gpus.size - 1;
			}
		}

		assert(_gpu != U32_MAX);

		_window->vk_swapchain_create();
		pipeline("../assets/shaders/triangle");

		auto& gpu = main_gpu();
		_cmdpool = vk_cmdpool(gpu, gpu.graphics.family);
	}

	vk_device::~vk_device() {
		auto& gpu = main_gpu();
		vkDeviceWaitIdle(gpu.device);

		_cmdpool.destroy(gpu);

		vkDestroyPipelineLayout(gpu.device, _pipeline.layout, nullptr);
		vkDestroyRenderPass(gpu.device, _pipeline.renderpass, nullptr);
		vkDestroyPipeline(gpu.device, _pipeline.pipeline, nullptr);

		_window->vk_term();

		for (auto& gpu : _gpus) {
			vkDestroyDevice(gpu.device, nullptr);
		}

		auto fn = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_instance, "vkDestroyDebugUtilsMessengerEXT");
		fn(_instance, _debugmsg, nullptr);
		vkDestroyInstance(_instance, nullptr);

		_pipeline.layout = VK_NULL_HANDLE;
		_debugmsg = VK_NULL_HANDLE;
		_instance = VK_NULL_HANDLE;
	}

	void vk_device::pipeline(cref<core::string> fname) {
		auto& gpu = main_gpu();

		auto fvertex = core::fopen(core::format_string("%.vert.spv", fname).data, core::access::ro);
		auto vbuf = fvertex.read();

		VkShaderModuleCreateInfo vinfo{};
		vinfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		vinfo.codeSize = vbuf.size;
		vinfo.pCode = (u32*)vbuf.data;

		auto ffragment = core::fopen(core::format_string("%.frag.spv", fname).data, core::access::ro);
		auto fbuf = ffragment.read();

		VkShaderModuleCreateInfo finfo{};
		finfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		finfo.codeSize = fbuf.size;
		finfo.pCode = (u32*)fbuf.data;

		VkShaderModule vmodule = VK_NULL_HANDLE, fmodule = VK_NULL_HANDLE;
		vkCreateShaderModule(gpu.device, &vinfo, nullptr, &vmodule);
		vkCreateShaderModule(gpu.device, &finfo, nullptr, &fmodule);

		VkPipelineShaderStageCreateInfo vstage{};
		vstage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vstage.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vstage.module = vmodule;
		vstage.pName = "main";

		VkPipelineShaderStageCreateInfo fstage{};
		fstage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fstage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fstage.module = fmodule;
		fstage.pName = "main";

		VkPipelineShaderStageCreateInfo shader_info[] = { vstage, fstage };

		core::vector<VkDynamicState> dynamic = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamic_info{};
		dynamic_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamic_info.dynamicStateCount = dynamic.size;
		dynamic_info.pDynamicStates = dynamic.data;

		VkPipelineVertexInputStateCreateInfo vertex_info{};
		vertex_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertex_info.vertexBindingDescriptionCount = 0;
		vertex_info.pVertexBindingDescriptions = nullptr;
		vertex_info.vertexAttributeDescriptionCount = 0;
		vertex_info.pVertexAttributeDescriptions = nullptr;

		VkPipelineInputAssemblyStateCreateInfo assembly_info{};
		assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		assembly_info.primitiveRestartEnable = VK_FALSE;

		VkPipelineViewportStateCreateInfo viewport_info{};
		viewport_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewport_info.viewportCount = 1;
		viewport_info.scissorCount = 1;

		VkPipelineRasterizationStateCreateInfo rasterization_info{};
		rasterization_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterization_info.depthClampEnable = VK_FALSE;
		rasterization_info.rasterizerDiscardEnable = VK_FALSE;
		rasterization_info.polygonMode = VK_POLYGON_MODE_FILL;
		rasterization_info.lineWidth = 1;
		rasterization_info.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterization_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterization_info.depthBiasEnable = VK_FALSE;
		rasterization_info.depthBiasConstantFactor = 0;
		rasterization_info.depthBiasClamp = 0;
		rasterization_info.depthBiasSlopeFactor = 0;

		VkPipelineMultisampleStateCreateInfo multisample_info{};
		multisample_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisample_info.sampleShadingEnable = VK_FALSE;
		multisample_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisample_info.minSampleShading = 1;
		multisample_info.pSampleMask = nullptr;
		multisample_info.alphaToCoverageEnable = VK_FALSE;
		multisample_info.alphaToOneEnable = VK_FALSE;

		VkPipelineDepthStencilStateCreateInfo depthstencil_info{};
		depthstencil_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

		VkPipelineColorBlendAttachmentState colorblend{};
		colorblend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorblend.blendEnable = VK_FALSE;
		colorblend.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorblend.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorblend.colorBlendOp = VK_BLEND_OP_ADD;
		colorblend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorblend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorblend.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo colorblend_info{};
		colorblend_info.sType= VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorblend_info.logicOpEnable = VK_FALSE;
		colorblend_info.logicOp = VK_LOGIC_OP_COPY;
		colorblend_info.attachmentCount = 1;
		colorblend_info.pAttachments = &colorblend;
		colorblend_info.blendConstants[0] = 0;
		colorblend_info.blendConstants[1] = 0;
		colorblend_info.blendConstants[2] = 0;
		colorblend_info.blendConstants[3] = 0;

		VkPipelineLayoutCreateInfo layout_info{};
		layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layout_info.setLayoutCount = 0;
		layout_info.pSetLayouts = nullptr;
		layout_info.pushConstantRangeCount = 0;
		layout_info.pPushConstantRanges = nullptr;

		vkCreatePipelineLayout(gpu.device, &layout_info, nullptr, &_pipeline.layout);

		VkAttachmentDescription color_attachment{};
		color_attachment.format = _window->surface->format;
		color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference color_attachmentref{};
		color_attachmentref.attachment = 0;
		color_attachmentref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &color_attachmentref;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderpass_info{};
		renderpass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderpass_info.attachmentCount = 1;
		renderpass_info.pAttachments = &color_attachment;
		renderpass_info.subpassCount = 1;
		renderpass_info.pSubpasses = &subpass;
		renderpass_info.dependencyCount = 1;
		renderpass_info.pDependencies = &dependency;

		vkCreateRenderPass(gpu.device, &renderpass_info, nullptr, &_pipeline.renderpass);

		VkGraphicsPipelineCreateInfo pipeline_info{};
		pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline_info.stageCount = 2;
		pipeline_info.pStages = shader_info;
		pipeline_info.pVertexInputState = &vertex_info;
		pipeline_info.pInputAssemblyState = &assembly_info;
		pipeline_info.pViewportState = &viewport_info;
		pipeline_info.pRasterizationState = &rasterization_info;
		pipeline_info.pMultisampleState = &multisample_info;
		pipeline_info.pDepthStencilState = nullptr;
		pipeline_info.pColorBlendState = &colorblend_info;
		pipeline_info.pDynamicState = &dynamic_info;
		pipeline_info.layout = _pipeline.layout;
		pipeline_info.renderPass = _pipeline.renderpass;
		pipeline_info.subpass = 0;
		pipeline_info.basePipelineHandle = nullptr;
		pipeline_info.basePipelineIndex = -1;

		vkCreateGraphicsPipelines(gpu.device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &_pipeline.pipeline);

		auto createframebufferscb = [](ref<window> state, u32 id, win_event event) {
			auto& surface = state.surface.ref();
			auto& swapchain = surface.surfaces[id];
			auto& gpu = surface.device.main_gpu();
			swapchain.framebuffers = core::vector<VkFramebuffer>(swapchain.views.size);
			for (i32 i : core::range(swapchain.views.size)) {
				VkImageView attachments[] = {
					swapchain.views[i]
				};

				VkFramebufferCreateInfo info{};
				info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				info.renderPass = surface.device._pipeline.renderpass;
				info.attachmentCount = 1;
				info.pAttachments = attachments;
				info.width = swapchain.extent.width;
				info.height = swapchain.extent.height;
				info.layers = 1;

				VkFramebuffer fb = VK_NULL_HANDLE;
				vkCreateFramebuffer(gpu.device, &info, nullptr, &fb);
				swapchain.framebuffers.add(fb);
			}
		};

		for (u32 id : _window->windows.keys()) {
			createframebufferscb(_window.ref(), id, win_event::create);
		}

		_window->addcb(win_event::create, createframebufferscb);

		vkDestroyShaderModule(gpu.device, vmodule, nullptr);
		vkDestroyShaderModule(gpu.device, fmodule, nullptr);
	}

	ref<vk_gpu> vk_device::main_gpu() const {
		return _gpus[_gpu];
	}

	void vk_device::step(f32 ms) {
		_window->step(ms);

		auto& gpu = main_gpu();
		auto& surface = _window->surface.ref();
		vkWaitForFences(gpu.device, 1, &surface.fence[_frame], VK_TRUE, U64_MAX);
		vkResetFences(gpu.device, 1, &surface.fence[_frame]);

		core::vector<VkCommandBuffer> cmdbufs(0);
		core::vector<VkSemaphore> wait_semaphores(0);
		core::vector<VkSemaphore> signal_semaphores(0);
		core::vector<VkPipelineStageFlags> wait_stages(0);
		core::vector<VkSwapchainKHR> swapchains(0);
		core::vector<u32> image_indices(0);
		for (u32 i : _window->windows.keys()) {
			VkCommandBuffer cmdbuf = _cmdpool.create(gpu);
			vkResetCommandBuffer(cmdbuf, 0);

			VkCommandBufferBeginInfo begin_info{};
			begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			begin_info.flags = 0;
			begin_info.pInheritanceInfo = nullptr;
			vkBeginCommandBuffer(cmdbuf, &begin_info);

			auto& swapchain = surface.surfaces[i];
			u32 image_index = U32_MAX;
			vkAcquireNextImageKHR(gpu.device, swapchain.swapchain, U64_MAX, swapchain.image_available[_frame], VK_NULL_HANDLE, &image_index);
			VkRenderPassBeginInfo renderpass_info{};
			renderpass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderpass_info.renderPass = _pipeline.renderpass;
			renderpass_info.framebuffer = swapchain.framebuffers[image_index];
			renderpass_info.renderArea.offset = { 0, 0 };
			renderpass_info.renderArea.extent = swapchain.extent;

			VkClearValue clear = {{{ 0.0f, 0.0f, 0.0f, 1.0f }}};
			renderpass_info.clearValueCount = 1;
			renderpass_info.pClearValues = &clear;

			vkCmdBeginRenderPass(cmdbuf, &renderpass_info, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline.pipeline);

			VkViewport viewport{};
			viewport.x = 0;
			viewport.y = 0;
			viewport.width = (float)swapchain.extent.width;
			viewport.height = (float)swapchain.extent.height;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
			vkCmdSetViewport(cmdbuf, 0, 1, &viewport);

			VkRect2D scissor{};
			scissor.offset = { 0, 0 };
			scissor.extent = swapchain.extent;
			vkCmdSetScissor(cmdbuf, 0, 1, &scissor);

			vkCmdDraw(cmdbuf, 3, 1, 0, 0);
			vkCmdEndRenderPass(cmdbuf);
			vkEndCommandBuffer(cmdbuf);

			cmdbufs.add(cmdbuf);
			wait_semaphores.add(swapchain.image_available[_frame]);
			signal_semaphores.add(swapchain.render_finished[_frame]);
			wait_stages.add(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
			swapchains.add(swapchain.swapchain);
			image_indices.add(image_index);

			_cmdpool.destroy(cmdbuf, surface.fence[_frame]);
		}

		VkSubmitInfo submit_info{};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.waitSemaphoreCount = wait_semaphores.size;
		submit_info.pWaitSemaphores = wait_semaphores.data;
		submit_info.pWaitDstStageMask = wait_stages.data;
		submit_info.commandBufferCount = cmdbufs.size;
		submit_info.pCommandBuffers = cmdbufs.data;
		submit_info.signalSemaphoreCount = signal_semaphores.size;
		submit_info.pSignalSemaphores = signal_semaphores.data;

		vkQueueSubmit(gpu.graphics.queue, 1, &submit_info, surface.fence[_frame]);

		VkPresentInfoKHR present_info{};
		present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		present_info.waitSemaphoreCount = signal_semaphores.size;
		present_info.pWaitSemaphores = signal_semaphores.data;
		present_info.swapchainCount = swapchains.size;
		present_info.pSwapchains = swapchains.data;
		present_info.pImageIndices = image_indices.data;
		present_info.pResults = nullptr;

		if (swapchains.size)
			vkQueuePresentKHR(gpu.present.queue, &present_info);

		_frame = (_frame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	vk_cmdpool::vk_cmdpool(cref<vk_gpu> gpu, u32 family)
	: pool(VK_NULL_HANDLE)
	, free(0)
	, busy(0) {
		VkCommandPoolCreateInfo pool_info{};
		pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		pool_info.queueFamilyIndex = family;
		vkCreateCommandPool(gpu.device, &pool_info, nullptr, &pool);
	}

	void vk_cmdpool::destroy(cref<vk_gpu> gpu) {
		vkDestroyCommandPool(gpu.device, pool, nullptr);
	}

	VkCommandBuffer vk_cmdpool::create(cref<vk_gpu> gpu) {
		for (i32 i : core::range(busy.size, 0, -1)) {
			i = i - 1;
			auto [buffer, fence] = busy[i];
			if (vkGetFenceStatus(gpu.device, fence) == VK_SUCCESS) {
				free.add(buffer);
				busy.del(i);
			}
		}

		if (free.size == 0) {
			VkCommandBufferAllocateInfo alloc_info{};
			alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			alloc_info.commandPool = pool;
			alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			alloc_info.commandBufferCount = 1;

			VkCommandBuffer buffer = VK_NULL_HANDLE;
			vkAllocateCommandBuffers(gpu.device, &alloc_info, &buffer);
			return buffer;
		}

		return free[--free.size];
	}

	void vk_cmdpool::destroy(VkCommandBuffer buf, VkFence fence) {
		busy.add({buf, fence});
	}
}
